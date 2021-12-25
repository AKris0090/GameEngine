#include "VulkanRenderer.h"
#include "vulkan/vulkan.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <stb_image.h>
#include <array>

#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#ifdef NDEBUG
const bool enableValLayers = false;
#else
const bool enableValLayers = true;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
DEBUG AND DEBUG MESSENGER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// Create the debug messenger using createInfo struct
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Fill the debug messenger with information to call back
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// Debug messenger creation and population called here
void VulkanRenderer::setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) {
    if (enableValLayers) {

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }
    else {
        return;
    }
}

// After debug messenger is used, destroy
void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
VALIDATION LAYER SUPPORT
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Check if validation layers requested are supported by querying the available layers
bool VulkanRenderer::checkValLayerSupport() {
    uint32_t numLayers;
    vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

    std::vector<VkLayerProperties> available(numLayers);
    vkEnumerateInstanceLayerProperties(&numLayers, available.data());

    for (const char* name : validationLayers) {
        bool found = false;

        // Check if layer is found using strcmp
        for (const auto& layerProps : available) {
            if (strcmp(name, layerProps.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE VULKAN INSTANCE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkInstance VulkanRenderer::createVulkanInstance(SDL_Window* window, const char* appName) {
    if ((enableValLayers == true) && (checkValLayerSupport() == false)) {
        std::_Xruntime_error("Validation layers were requested, but none were available");
    }

    // Get application information for the create info struct
    VkApplicationInfo aInfo{};
    aInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    aInfo.pApplicationName = appName;
    aInfo.applicationVersion = 1;
    aInfo.pEngineName = "No Engine";
    aInfo.engineVersion = 1;
    aInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCInfo{};
    instanceCInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCInfo.pApplicationInfo = &aInfo;

    // Poll extensions and add to create info struct
    unsigned int extensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr)) {
        std::_Xruntime_error("Unable to figure out the number of vulkan extensions!");
    }

    std::vector<const char*> extNames(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extNames.data())) {
        std::_Xruntime_error("Unable to figure out the vulkan extension names!");
    }

    // Add the validation layers extension to the extension names array
    if (enableValLayers) {
        extNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Fill create info struct with extension count and name information
    instanceCInfo.enabledExtensionCount = extNames.size();
    instanceCInfo.ppEnabledExtensionNames = extNames.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    // If using validation layers, add information to the create info struct
    if (enableValLayers) {
        instanceCInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        instanceCInfo.enabledLayerCount = 0;
    }

    // Create the instance
    if (vkCreateInstance(&instanceCInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    return instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE SDL SURFACE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createSurface(SDL_Window* window) {
    SDL_Vulkan_CreateSurface(window, instance, &surface);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CHECKING EXENSION SUPPORT
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderer::checkExtSupport(VkPhysicalDevice physicalDevice) {
    // Poll available extensions provided by the physical device and then check if required extensions are among them
    uint32_t numExts;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExts, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(numExts);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExts, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExts.begin(), deviceExts.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // Return true or false depending on if the required extensions are fulfilled
    return requiredExtensions.empty();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
SWAP CHAIN METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// First aspect : image format
VkSurfaceFormatKHR VulkanRenderer::SWChainSuppDetails::chooseSwSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        // If it has the right color space and format, return it
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // Otherwise, return the first specified format
    return availableFormats[0];
}

// Second aspect : present mode
VkPresentModeKHR VulkanRenderer::SWChainSuppDetails::chooseSwPresMode(const std::vector<VkPresentModeKHR>& availablePresModes) {
    // Using Mailbox present mode, if possible
    // Can also use: VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIF_RELAXED_KHR, or VK_PRESENT_MODE_MAILBOX_KHR
    for (const VkPresentModeKHR& availablePresMode : availablePresModes) {
        if (availablePresMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresMode;
        }
    }

    // If device does not support it, use First-In-First-Out present mode
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Third aspect : image extent
VkExtent2D VulkanRenderer::SWChainSuppDetails::chooseSwExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        SDL_GL_GetDrawableSize(window, &width, &height);

        VkExtent2D actExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actExtent.width = std::clamp(actExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actExtent.height = std::clamp(actExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actExtent;
    }
}

// Get details of the capabilities, formats, and presentation modes avialable from the physical device
VulkanRenderer::SWChainSuppDetails VulkanRenderer::getDetails(VkPhysicalDevice physicalDevice) {
    SWChainSuppDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// Actual creation of the swap chain
void VulkanRenderer::createSWChain(SDL_Window* window) {
    SWChainSuppDetails swInfo = getDetails(GPU);

    VkSurfaceFormatKHR surfaceFormat = swInfo.chooseSwSurfaceFormat(swInfo.formats);
    VkPresentModeKHR presentMode = swInfo.chooseSwPresMode(swInfo.presentModes);
    VkExtent2D extent = swInfo.chooseSwExtent(swInfo.capabilities, window);

    // Also set the number of images we want in the swap chain, accomodate for driver to complete internal operations before we can acquire another image to render to
    uint32_t numImages = swInfo.capabilities.minImageCount + 1;

    // Make sure to not exceed maximum number of images: 0 means no maximum
    if (swInfo.capabilities.maxImageCount > 0 && numImages > swInfo.capabilities.maxImageCount) {
        numImages = swInfo.capabilities.maxImageCount;
    }

    // Fill in the structure to create the swap chain object
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;

    // Details of swap chain images
    swapchainCreateInfo.minImageCount = numImages;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    // Unless developing a stereoscopic 3D image, this is always 1
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // Specify how images from swap chain are handled
    // If graphics queue family is different from the present queue family, draw on the images in swap chain from graphics and submit them on present
    QueueFamilyIndices indices = findQueueFamilies(GPU);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // For transforming the image (rotation, translation, etc.)
    swapchainCreateInfo.preTransform = swInfo.capabilities.currentTransform;

    // Alpha channel is used for blending with other windows, so ignore
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Unless you want to be able to read pixels that are obscured because of another window in front of them, set clipped to VK_TRUE
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    // If the old swap chain becomes invalid, for now set to null
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    PFN_vkCreateDebugReportCallbackEXT;

    VkResult res = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &numImages, nullptr);
    SWChainImages.resize(numImages);
    vkGetSwapchainImagesKHR(device, swapChain, &numImages, SWChainImages.data());

    SWChainImageFormat = surfaceFormat.format;
    SWChainExtent = extent;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
PHYSICAL DEVICE AND LOGICAL DEVICE SELECTION AND CREATION METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderer::isSuitable(VkPhysicalDevice physicalDevice) {
    // If specific feature is needed, then poll for it, otherwise just return true for any suitable Vulkan supported GPU
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // Make sure the extensions are supported using the method defined above
    bool extsSupported = checkExtSupport(physicalDevice);

    // Make sure the physical device is capable of supporting a swap chain with the right formats and presentation modes
    bool isSWChainAdequate = false;
    if (extsSupported) {
        SWChainSuppDetails SWChainSupp = getDetails(physicalDevice);
        isSWChainAdequate = !SWChainSupp.formats.empty() && !SWChainSupp.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures );

    return (indices.isComplete() && extsSupported && isSWChainAdequate && supportedFeatures.samplerAnisotropy);
}

// Parse through the list of available physical devices and choose the one that is suitable
void VulkanRenderer::pickPhysicalDevice() {
    // Enumerate physical devices and store it in a variable, and check if there are none available
    uint32_t numDevices = 0;
    vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
    if (numDevices == 0) {
        std::_Xruntime_error("no GPUs found with Vulkan support!");
    }

    // Otherwise, store all handles in array
    std::vector<VkPhysicalDevice> devices(numDevices);
    vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());

    // Evaluate each, check if they are suitable
    for (const auto& device : devices) {
        if (isSuitable(device)) {
            GPU = device;
            break;
        }
    }

    if (GPU == VK_NULL_HANDLE) {
        std::_Xruntime_error("could not find a suitable GPU!");
    }
}

// Setting up the logical device using the physical device
void VulkanRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(GPU);

    // Create presentation queue with structs
    std::vector<VkDeviceQueueCreateInfo> queuecInfos;
    std::set<uint32_t> uniqueQFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePrio = 1.0f;
    for (uint32_t queueFamily : uniqueQFamilies) {
        VkDeviceQueueCreateInfo queuecInfo{};
        queuecInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queuecInfo.queueFamilyIndex = queueFamily;
        queuecInfo.queueCount = 1;
        queuecInfo.pQueuePriorities = &queuePrio;
        queuecInfos.push_back(queuecInfo);
    }


    // Specifying device features through another struct
    VkPhysicalDeviceFeatures gpuFeatures{};
    gpuFeatures.samplerAnisotropy = VK_TRUE;

    // Add the feature structs on the pNext of the vkDeviceCreateInfo
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    rtPipelineFeature.pNext = &accelFeature;


    // Create the logical device, filling in with the create info structs
    VkDeviceCreateInfo deviceCInfo{};
    deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCInfo.pNext = &rtPipelineFeature;
    deviceCInfo.queueCreateInfoCount = static_cast<uint32_t>(queuecInfos.size());
    deviceCInfo.pQueueCreateInfos = queuecInfos.data();

    deviceCInfo.pEnabledFeatures = &gpuFeatures;

    // Set enabledLayerCount and ppEnabledLayerNames fields to be compatible with older implementations of Vulkan
    deviceCInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
    deviceCInfo.ppEnabledExtensionNames = deviceExts.data();

    // If validation layers are enabled, then fill create info struct with size and name information
    if (enableValLayers) {
        deviceCInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        deviceCInfo.enabledLayerCount = 0;
    }

    // Instantiate
    if (vkCreateDevice(GPU, &deviceCInfo, nullptr, &device) != VK_SUCCESS) {
        std::_Xruntime_error("failed to instantiate logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING AND HANDLING IMAGE VIEWS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createImageViews() {
    SWChainImageViews.resize(SWChainImages.size());

    for (uint32_t i = 0; i < SWChainImages.size(); i++) {
        SWChainImageViews[i] = createImageView(SWChainImages[i], SWChainImageFormat);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE RENDER PASS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = SWChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // Determine what to do with the data in the attachment before and after the rendering
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Apply to stencil data
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Specify the layout of pixels in memory for the images
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // One render pass consists of multiple render subpasses, but we are only going to use 1 for the triangle
    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create the subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // Have to be explicit about this subpass being a graphics subpass
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    // Create information struct for the render pass
    VkRenderPassCreateInfo renderPassCInfo{};
    renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCInfo.attachmentCount = 1;
    renderPassCInfo.pAttachments = &colorAttachment;
    renderPassCInfo.subpassCount = 1;
    renderPassCInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};

    // Specify the indices and the dependent subpass
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    // Specify operations to wait on and the stages in which they occur
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Add this information to the render pass create informatiton
    renderPassCInfo.dependencyCount = 1;
    renderPassCInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassCInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create render pass!");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
DESCRIPTOR SET LAYOUT
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding UBOLayoutBinding{};
    UBOLayoutBinding.binding = 0;
    UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UBOLayoutBinding.descriptorCount = 1;
    UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    UBOLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { UBOLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutCInfo{};
    layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutCInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutCInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor set layout!");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
GRAPHICS PIPELINE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createGraphicsPipeline() {
    // Read the file for the bytecodfe of the shaders
    std::vector<char> vertexShader = readFile("shaders/vert.spv");
    std::vector<char> fragmentShader = readFile("shaders/frag.spv");

    // Wrap the bytecode with VkShaderModule objects
    VkShaderModule vertexShaderModule = createShaderModule(vertexShader);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShader);

    //Create the shader information struct to begin actuall using the shader
    VkPipelineShaderStageCreateInfo vertextStageCInfo{};
    vertextStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertextStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertextStageCInfo.module = vertexShaderModule;
    vertextStageCInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStageCInfo{};
    fragmentStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageCInfo.module = fragmentShaderModule;
    fragmentStageCInfo.pName = "main";

    // Define array to contain the shader create information structs
    VkPipelineShaderStageCreateInfo stages[] = { vertextStageCInfo, fragmentStageCInfo };

    // Describing the format of the vertex data to be passed to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputCInfo{};
    vertexInputCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputCInfo.vertexBindingDescriptionCount = 1;
    vertexInputCInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
    vertexInputCInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Next struct describes what kind of geometry will be drawn from the verts and if primitive restart should be enabled
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCInfo{};
    inputAssemblyCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCInfo.primitiveRestartEnable = false;

    // Initialize the viewport information struct, a lot of the size information will come from the swap chain extent factor
    VkViewport viewPort{};
    viewPort.x = 0.0f;
    viewPort.y = 0.0f;
    viewPort.width = (float)SWChainExtent.width;
    viewPort.height = (float)SWChainExtent.height;
    viewPort.minDepth = 0.0f;
    viewPort.maxDepth = 1.0f;

    // Create scissor rectangle to cover the entireity of the viewport
    VkRect2D scissorRect{};
    scissorRect.offset = { 0, 0 };
    scissorRect.extent = SWChainExtent;

    // Combine the viewport information struct and the scissor rectangle into a single viewport state
    VkPipelineViewportStateCreateInfo viewportStateCInfo{};
    viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCInfo.viewportCount = 1;
    viewportStateCInfo.pViewports = &viewPort;
    viewportStateCInfo.scissorCount = 1;
    viewportStateCInfo.pScissors = &scissorRect;

    // Initialize rasterizer, which takes information from the geometry formed by the vertex shader into fragments to be colored by the fragment shader
    VkPipelineRasterizationStateCreateInfo rasterizerCInfo{};
    rasterizerCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // Fragments beyond the near and far planes are clamped to those planes, instead of discarding them
    rasterizerCInfo.depthClampEnable = VK_FALSE;
    // If set to true, geometry never passes through the rasterization phase, and disables output to framebuffer
    rasterizerCInfo.rasterizerDiscardEnable = VK_FALSE;
    // Determines how fragments are generated for geometry, using other modes requires enabling a GPU feature
    rasterizerCInfo.polygonMode = VK_POLYGON_MODE_FILL;

    // Linewidth describes thickness of lines in terms of number of fragments 
    rasterizerCInfo.lineWidth = 1.0f;
    // Specify type of culling and and the vertex order for the faces to be considered
    rasterizerCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // Alter depth values by adding constant or biasing them based on a fragment's slope
    rasterizerCInfo.depthBiasEnable = VK_FALSE;
    rasterizerCInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCInfo.depthBiasClamp = 0.0f;
    rasterizerCInfo.depthBiasSlopeFactor = 0.0f;

    // Multisampling information struct
    VkPipelineMultisampleStateCreateInfo multiSamplingCInfo{};
    multiSamplingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSamplingCInfo.sampleShadingEnable = VK_FALSE;
    multiSamplingCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multiSamplingCInfo.minSampleShading = 1.0f;
    multiSamplingCInfo.pSampleMask = nullptr;
    multiSamplingCInfo.alphaToCoverageEnable = VK_FALSE;
    multiSamplingCInfo.alphaToOneEnable = VK_FALSE;

    // Depth and stencil testing would go here, but not doing this for the triangle


    // Color blending - color from fragment shader needs to be combined with color already in the framebuffer
    // If <blendEnable> is set to false, then the color from the fragment shader is passed through to the framebuffer
    // Otherwise, combine with a colorWriteMask to determine the channels that are passed through
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    // Array of structures for all of the framebuffers to set blend constants as blend factors
    VkPipelineColorBlendStateCreateInfo colorBlendingCInfo{};

    if (colorBlendEnable) {
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        colorBlendingCInfo.logicOpEnable = VK_FALSE;
    }
    else {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        colorBlendingCInfo.logicOpEnable = VK_TRUE;
    }
    colorBlendingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingCInfo.attachmentCount = 1;
    colorBlendingCInfo.pAttachments = &colorBlendAttachment;
    colorBlendingCInfo.blendConstants[0] = 0.0f;
    colorBlendingCInfo.blendConstants[1] = 0.0f;
    colorBlendingCInfo.blendConstants[2] = 0.0f;
    colorBlendingCInfo.blendConstants[3] = 0.0f;

    // Not much can be changed without completely recreating the rendering pipeline, so we fill in a struct with the information
    VkDynamicState dynaStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCInfo{};   
    dynamicStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCInfo.dynamicStateCount = 2;
    dynamicStateCInfo.pDynamicStates = dynaStates;

    // We can use uniform values to make changes to the shaders without having to create them again, similar to global variables
    // Initialize the pipeline layout with another create info struct
    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = 1;
    pipeLineLayoutCInfo.pSetLayouts = &descriptorSetLayout;
    pipeLineLayoutCInfo.pushConstantRangeCount = 0;
    pipeLineLayoutCInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipeLineLayoutCInfo, nullptr, &pipeLineLayout) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create pipeline layout!");
    }

    // Combine the shader stages, fixed-function state, pipeline layout, and render pass to create the graphics pipeline
    // First - populate struct with the information
    VkGraphicsPipelineCreateInfo graphicsPipelineCInfo{};
    graphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCInfo.stageCount = 2;
    graphicsPipelineCInfo.pStages = stages;

    graphicsPipelineCInfo.pVertexInputState = &vertexInputCInfo;
    graphicsPipelineCInfo.pInputAssemblyState = &inputAssemblyCInfo;
    graphicsPipelineCInfo.pViewportState = &viewportStateCInfo;
    graphicsPipelineCInfo.pRasterizationState = &rasterizerCInfo;
    graphicsPipelineCInfo.pMultisampleState = &multiSamplingCInfo;
    graphicsPipelineCInfo.pDepthStencilState = nullptr;
    graphicsPipelineCInfo.pColorBlendState = &colorBlendingCInfo;
    graphicsPipelineCInfo.pDynamicState = nullptr;

    graphicsPipelineCInfo.layout = pipeLineLayout;

    graphicsPipelineCInfo.renderPass = renderPass;
    graphicsPipelineCInfo.subpass = 0;

    graphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCInfo.basePipelineIndex = -1;

    // Create the object
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the graphics pipeline!");
    }

    // After all the processing with the modules is over, destroy them
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
HELPER METHODS FOR THE GRAPHICS PIPELINE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<char> VulkanRenderer::readFile(const std::string& filename) {
    // Start reading at end of the file and read as binary
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::_Xruntime_error("Failed to open the specified file!");
    }

    // Read the file, create the buffer, and return it
    size_t fileSize = file.tellg();
    std::vector<char> buffer((size_t)file.tellg());
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

// In order to pass the binary code to the graphics pipeline, we need to create a VkShaderModule object to wrap it with
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& binary) {
    // We need to specify a pointer to the buffer with the bytecode and the length of the bytecode. Bytecode pointer is a uint32_t pointer
    VkShaderModuleCreateInfo shaderModuleCInfo{};
    shaderModuleCInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCInfo.codeSize = binary.size();
    shaderModuleCInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

    VkShaderModule shaderMod;
    if (vkCreateShaderModule(device, &shaderModuleCInfo, nullptr, &shaderMod)) {
        std::_Xruntime_error("Failed to create a shader module!");
    }

    return shaderMod;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE FRAME BUFFER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createFrameBuffer() {
    SWChainFrameBuffers.resize(SWChainImageViews.size());

    // Iterate through the image views and create framebuffers from them
    for (size_t i = 0; i < SWChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            SWChainImageViews[i]
        };

        VkFramebufferCreateInfo frameBufferCInfo{};
        frameBufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCInfo.renderPass = renderPass;
        frameBufferCInfo.attachmentCount = 1;
        frameBufferCInfo.pAttachments = attachments;
        frameBufferCInfo.width = SWChainExtent.width;
        frameBufferCInfo.height = SWChainExtent.height;
        frameBufferCInfo.layers = 1;

        if (vkCreateFramebuffer(device, &frameBufferCInfo, nullptr, &SWChainFrameBuffers[i]) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to create a framebuffer for an image view!");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE THE VERTEX, INDEX, AND UNIFORM BUFFERS AND OTHER HELPER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::_Xruntime_error("Failed to find a suitable memory type!");
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferCInfo{};
    bufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCInfo.size = size;
    bufferCInfo.usage = usage;
    bufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferCInfo, nullptr, &buffer) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the vertex buffer!");
    }

    vkGetPhysicalDeviceMemoryProperties(GPU, &memProperties);
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(SWChainImages.size());
    uniformBuffersMemory.resize(SWChainImages.size());

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void VulkanRenderer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(SWChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(SWChainImages.size());

    VkDescriptorPoolCreateInfo poolCInfo{};
    poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCInfo.pPoolSizes = poolSizes.data();
    poolCInfo.maxSets = static_cast<uint32_t>(SWChainImages.size());

    if (vkCreateDescriptorPool(device, &poolCInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor pool!");
    }
}

void VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(SWChainImages.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool;
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(SWChainImages.size());
    allocateInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(SWChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        VkDescriptorBufferInfo descriptorBufferInfo{};
        descriptorBufferInfo.buffer = uniformBuffers[i];
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWriteSets{};

        descriptorWriteSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteSets[0].dstSet = descriptorSets[i];
        descriptorWriteSets[0].dstBinding = 0;
        descriptorWriteSets[0].dstArrayElement = 0;
        descriptorWriteSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteSets[0].descriptorCount = 1;
        descriptorWriteSets[0].pBufferInfo = &descriptorBufferInfo;

        descriptorWriteSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteSets[1].dstSet = descriptorSets[i];
        descriptorWriteSets[1].dstBinding = 1;
        descriptorWriteSets[1].dstArrayElement = 0;
        descriptorWriteSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteSets[1].descriptorCount = 1;
        descriptorWriteSets[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE COMMAND POOL AND BUFFER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createCommandPool() {
    QueueFamilyIndices QFIndices = findQueueFamilies(GPU);

    // Creating the command pool create information struct
    VkCommandPoolCreateInfo commandPoolCInfo{};
    commandPoolCInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCInfo.queueFamilyIndex = QFIndices.graphicsFamily.value();
    commandPoolCInfo.flags = 0;

    // Actual creation of the command buffer
    if (vkCreateCommandPool(device, &commandPoolCInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create a command pool!");
    }
}

void VulkanRenderer::createCommandBuffers() {
    commandBuffers.resize(SWChainFrameBuffers.size());

    // Information to allocate the frame buffer
    VkCommandBufferAllocateInfo CBAllocateInfo{};
    CBAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CBAllocateInfo.commandPool = commandPool;
    CBAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CBAllocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &CBAllocateInfo, commandBuffers.data()) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate a command buffer!");
    }

    // Start command buffer recording
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo CBBeginInfo{};
        CBBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CBBeginInfo.flags = 0;
        CBBeginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffers[i], &CBBeginInfo) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to start recording with the command buffer!");
        }

        // Start the render pass
        VkRenderPassBeginInfo RPBeginInfo{};
        // Create the render pass
        RPBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RPBeginInfo.renderPass = renderPass;
        RPBeginInfo.framebuffer = SWChainFrameBuffers[i];

        // Define the size of the render area
        RPBeginInfo.renderArea.offset = { 0, 0 };
        RPBeginInfo.renderArea.extent = SWChainExtent;

        // Define the clear values to use
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        RPBeginInfo.clearValueCount = 1;
        RPBeginInfo.pClearValues = &clearColor;

        // Finally, begin the render pass
        vkCmdBeginRenderPass(commandBuffers[i], &RPBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind the graphics pipeline, and instruct it to draw the triangle
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        // After drawing is over, end the render pass
        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to record back the command buffer!");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
INITIALIZING THE TWO SEMAPHORES AND THE FENCES
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createSemaphores(const int maxFramesInFlight) {
    imageAcquiredSema.resize(maxFramesInFlight);
    renderedSema.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);
    imagesInFlight.resize(SWChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaCInfo{};
    semaCInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCInfo{};
    fenceCInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device, &semaCInfo, nullptr, &imageAcquiredSema[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaCInfo, nullptr, &renderedSema[i]) != VK_SUCCESS || vkCreateFence(device, &fenceCInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to create the synchronization objects for a frame!");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
IMAGE TEXTURES
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageCInfo{};
    imageCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCInfo.extent.width = width;
    imageCInfo.extent.height = height;
    imageCInfo.extent.depth = 1;
    imageCInfo.mipLevels = 1;
    imageCInfo.arrayLayers = 1;

    imageCInfo.format = format;
    imageCInfo.tiling = tiling;
    imageCInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCInfo.usage = usage;
    imageCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCInfo.flags = 0;

    if (vkCreateImage(device, &imageCInfo, nullptr, &image) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create an image!");
    }

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate the image memory!");
    }

    vkBindImageMemory(device, textureImage, textureImageMemory, 0);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::createTextureImage() {
    int textureWidth, textureHeight, texChannels;
    stbi_uc* pixels = stbi_load("Images/texture.jpg", &textureWidth, &textureHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize currentImageSize = textureWidth * textureHeight * 4;

    if (!pixels) {
        std::_Xruntime_error("Failed to load the texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(currentImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, currentImageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(currentImageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo imageViewCInfo{};
    imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCInfo.image = image;
    imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCInfo.format = format;
    imageViewCInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCInfo.subresourceRange.baseMipLevel = 0;
    imageViewCInfo.subresourceRange.levelCount = 1;
    imageViewCInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCInfo.subresourceRange.layerCount = 1;

    VkImageView tempImageView;
    if (vkCreateImageView(device, &imageViewCInfo, nullptr, &tempImageView) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create a texture image view!");
    }

    return tempImageView;
}

void VulkanRenderer::createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);

}

void VulkanRenderer::createTextureImageSampler() {
    VkSamplerCreateInfo samplerCInfo{};
    samplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCInfo.magFilter = VK_FILTER_LINEAR;
    samplerCInfo.minFilter = VK_FILTER_LINEAR;
    samplerCInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(GPU, &properties);
    samplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCInfo.compareEnable = VK_FALSE;
    samplerCInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCInfo.mipLodBias = 0.0f;
    samplerCInfo.minLod = 0.0f;
    samplerCInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerCInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
SWAPCHAIN RECREATION
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::cleanupSWChain() {
    for (size_t i = 0; i < SWChainFrameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, SWChainFrameBuffers[i], nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeLineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < SWChainImageViews.size(); i++) {
        vkDestroyImageView(device, SWChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void VulkanRenderer::recreateSwapChain(SDL_Window* window) {
    vkDeviceWaitIdle(device);

    cleanupSWChain();

    createSWChain(window);
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
RAYTRACING METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::initializeRT() {
    VkPhysicalDeviceProperties2 deviceProps2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    deviceProps2.pNext = &physicalDeviceRTProperties;
    vkGetPhysicalDeviceProperties2(GPU, &deviceProps2);
}