#include "volk-master/volk.h"
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
#include <array>
#include "glm-0.9.6.3/glm.hpp"
#include <unordered_map>
#include "RayTrace.h"

#include "tinyobjloader-master/tiny_obj_loader.h"

#include "stb-master/stb_image.h"

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
VkResult RayTrace::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Fill the debug messenger with information to call back
void RayTrace::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// Debug messenger creation and population called here
void RayTrace::setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) {
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
void RayTrace::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
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
bool RayTrace::checkValLayerSupport() {
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

VkInstance RayTrace::createVulkanInstance(SDL_Window* window, const char* appName) {
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
    aInfo.apiVersion = VK_API_VERSION_1_2;

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
    instanceCInfo.enabledExtensionCount = static_cast<uint32_t>(extNames.size());
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

void RayTrace::createSurface(SDL_Window* window) {
    SDL_Vulkan_CreateSurface(window, instance, &surface);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CHECKING EXENSION SUPPORT
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RayTrace::checkExtSupport(VkPhysicalDevice physicalDevice) {
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
VkSurfaceFormatKHR RayTrace::SWChainSuppDetails::chooseSwSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
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
VkPresentModeKHR RayTrace::SWChainSuppDetails::chooseSwPresMode(const std::vector<VkPresentModeKHR>& availablePresModes) {
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
VkExtent2D RayTrace::SWChainSuppDetails::chooseSwExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
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
RayTrace::SWChainSuppDetails RayTrace::getDetails(VkPhysicalDevice physicalDevice) {
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
void RayTrace::createSWChain(SDL_Window* window) {
    VkFenceCreateInfo fenceCInfo{};
    fenceCInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(device, &fenceCInfo, nullptr, &commandFence);

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

bool RayTrace::isSuitable(VkPhysicalDevice physicalDevice) {
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
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    return (indices.isComplete() && extsSupported && isSWChainAdequate && supportedFeatures.samplerAnisotropy);
}

// Parse through the list of available physical devices and choose the one that is suitable
void RayTrace::pickPhysicalDevice() {
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
void RayTrace::createLogicalDevice() {
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

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{};
    accelFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelFeature.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{};
    rtPipelineFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rtPipelineFeature.rayTracingPipeline = VK_TRUE;
    rtPipelineFeature.pNext = &accelFeature;

    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR BDAfeature{};
    BDAfeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    BDAfeature.bufferDeviceAddress = VK_TRUE;
    BDAfeature.pNext = &rtPipelineFeature;

    VkPhysicalDeviceHostQueryResetFeaturesEXT resetHQfeature{};
    resetHQfeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT;
    resetHQfeature.hostQueryReset = VK_TRUE;
    resetHQfeature.pNext = &BDAfeature;

    // Create the logical device, filling in with the create info structs
    VkDeviceCreateInfo deviceCInfo{};
    deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCInfo.pNext = &resetHQfeature;
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

void RayTrace::createImageViews() {
    SWChainImageViews.resize(SWChainImages.size());

    for (uint32_t i = 0; i < SWChainImages.size(); i++) {
        SWChainImageViews[i] = createImageView(SWChainImages[i], SWChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE RENDER PASS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RayTrace::createRenderPass() {
    VkAttachmentDescription colorAttachmentDescription{};
    colorAttachmentDescription.format = SWChainImageFormat;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

    // Determine what to do with the data in the attachment before and after the rendering
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Apply to stencil data
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Specify the layout of pixels in memory for the images
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // One render pass consists of multiple render subpasses, but we are only going to use 1 for the triangle
    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachmentDescription{};
    depthAttachmentDescription.format = findDepthFormat();
    depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference{};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Create the subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // Have to be explicit about this subpass being a graphics subpass
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    VkSubpassDependency dependency{};
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Create information struct for the render pass
    std::array<VkAttachmentDescription, 2> attachments = { colorAttachmentDescription, depthAttachmentDescription };
    VkRenderPassCreateInfo renderPassCInfo{};
    renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCInfo.pAttachments = attachments.data();
    renderPassCInfo.subpassCount = 1;
    renderPassCInfo.pSubpasses = &subpass;
    renderPassCInfo.dependencyCount = 1;
    renderPassCInfo.pDependencies = &dependency;

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

void RayTrace::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding UBOLayoutBinding{};
    UBOLayoutBinding.binding = 0;
    UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UBOLayoutBinding.descriptorCount = 1;
    UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    UBOLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding rayTraceBinding{};
    rayTraceBinding.binding = 2;
    rayTraceBinding.descriptorCount = 1;
    rayTraceBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    rayTraceBinding.pImmutableSamplers = nullptr;
    rayTraceBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding outImageBinding{};
    outImageBinding.binding = 3;
    outImageBinding.descriptorCount = 1;
    outImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    outImageBinding.pImmutableSamplers = nullptr;
    outImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 4> bindings = { UBOLayoutBinding, samplerLayoutBinding, rayTraceBinding, outImageBinding };
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

void RayTrace::createGraphicsPipeline() {
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
    rasterizerCInfo.cullMode = VK_CULL_MODE_NONE;
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

    VkPipelineDepthStencilStateCreateInfo depthStencilCInfo{};
    depthStencilCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCInfo.depthTestEnable = VK_TRUE;
    depthStencilCInfo.depthWriteEnable = VK_TRUE;
    depthStencilCInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCInfo.minDepthBounds = 0.0f;
    depthStencilCInfo.maxDepthBounds = 1.0f;

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

    graphicsPipelineCInfo.pDepthStencilState = &depthStencilCInfo;

    // Create the object
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCInfo, nullptr, &rayTracingPipeline) != VK_SUCCESS) {
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

std::vector<char> RayTrace::readFile(const std::string& filename) {
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
VkShaderModule RayTrace::createShaderModule(const std::vector<char>& binary) {
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

void RayTrace::createFrameBuffer() {
    SWChainFrameBuffers.resize(SWChainImageViews.size());

    // Iterate through the image views and create framebuffers from them
    for (size_t i = 0; i < SWChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = { SWChainImageViews[i], depthImageView };

        VkFramebufferCreateInfo frameBufferCInfo{};
        frameBufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCInfo.renderPass = renderPass;
        frameBufferCInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferCInfo.pAttachments = attachments.data();
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
MODEL LOADER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RayTrace::loadModel(glm::mat4 transform) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    OBJInstance instance;
    instance.index = static_cast<uint32_t>(loadedModels.size());
    instance.transform = transform;
    instance.transformIT = glm::transpose(glm::inverse(transform));
    instance.textureOffset = static_cast<uint32_t>(materials.size());

    loadedModels.resize(static_cast<uint32_t>(loadedModels.size()) + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            RayTrace::Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            loadedModels[0].vertices.push_back(vertex);
            loadedModels[0].indices.push_back(static_cast<uint32_t>(loadedModels[0].indices.size()));
        }
    }

    instances.emplace_back(instance);

    numModels += 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE THE VERTEX, INDEX, AND UNIFORM BUFFERS AND OTHER HELPER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer RayTrace::beginSingleTimeCommands() {
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

void RayTrace::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkResetFences(device, 1, &commandFence);

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, commandFence);

    printf("submitted \n");

    vkWaitForFences(device, 1, &commandFence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    printf("Freed \n");
}

uint32_t RayTrace::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(GPU, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::_Xruntime_error("Failed to find a suitable memory type!");
}

void RayTrace::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferCInfo{};
    bufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCInfo.size = size;
    bufferCInfo.usage = usage;
    bufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferCInfo, nullptr, &buffer) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = &memoryAllocateFlagsInfo;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void RayTrace::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void RayTrace::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(loadedModels[0].vertices[0]) * loadedModels[0].vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, loadedModels[0].vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void RayTrace::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(loadedModels[0].indices[0]) * loadedModels[0].indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, loadedModels[0].indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void RayTrace::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(SWChainImages.size());
    uniformBuffersMemory.resize(SWChainImages.size());

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void RayTrace::createDescriptorPool() {
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

void RayTrace::createRTDescriptorSet() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &descriptorSetLayout;

    descriptorSets.resize(SWChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate descriptor sets!");
    }

    VkAccelerationStructureKHR TLAStructure = topLevelAccelStructure;

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        VkWriteDescriptorSetAccelerationStructureKHR descriptorSetAccelInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
        descriptorSetAccelInfo.accelerationStructureCount = 1;
        descriptorSetAccelInfo.pAccelerationStructures = &TLAStructure;
        VkDescriptorImageInfo imageInfo{ {}, SWChainImageViews[i], VK_IMAGE_LAYOUT_GENERAL }; //Careful with this

        VkDescriptorBufferInfo descriptorBufferInfo{};
        descriptorBufferInfo.buffer = uniformBuffers[i];
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(UniformBufferObject);

        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = storageImageView;

        std::vector<VkWriteDescriptorSet> descriptorWriteSets{};

        VkWriteDescriptorSet write{};
        write.dstSet = descriptorSet;

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &descriptorBufferInfo;

        descriptorWriteSets.push_back(write);

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets[i];
        write.dstBinding = 1;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        descriptorWriteSets.push_back(write);

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets[i];
        write.dstBinding = 2;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        write.pNext = &descriptorSetAccelInfo;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        descriptorWriteSets.push_back(write);

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets[i];
        write.dstBinding = 3;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        descriptorWriteSets.push_back(write);

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
    }
}

void RayTrace::createRtPipeline() {
    enum stageIndices
    {
        raygen,
        miss,
        closestHit,
        shaderGroupCount
    };
    std::array<VkPipelineShaderStageCreateInfo, shaderGroupCount> stages{};
    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.pName = "main";

    //Raygeneration
    std::vector<char> raygenShader = readFile("shaders/rgen.spv");
    stage.module = createShaderModule(raygenShader);
    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stages[raygen] = stage;

    //Miss
    std::vector<char> rayMissShader = readFile("shaders/rmiss.spv");
    stage.module = createShaderModule(rayMissShader);
    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages[miss] = stage;

    //ClosestHit
    std::vector<char> rayCHShader = readFile("shaders/rhit.spv");
    stage.module = createShaderModule(rayCHShader);
    stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages[closestHit] = stage;

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    // Raygen
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = raygen;
    shaderGroups.push_back(group);

    // Miss
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = miss;
    shaderGroups.push_back(group);

    // closest hit shader
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = closestHit;
    shaderGroups.push_back(group);

    VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstantRay) };

    // We can use uniform values to make changes to the shaders without having to create them again, similar to global variables
    // Initialize the pipeline layout with another create info struct
    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.pushConstantRangeCount = 1;
    pipeLineLayoutCInfo.pPushConstantRanges = &pushConstant;
    pipeLineLayoutCInfo.setLayoutCount = 1;
    pipeLineLayoutCInfo.pSetLayouts = &descriptorSetLayout;
    pipeLineLayoutCInfo.pushConstantRangeCount = 0;
    pipeLineLayoutCInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipeLineLayoutCInfo, nullptr, &pipeLineLayout) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create pipeline layout!");
    }

    // Assemble the shader stages and recursion depth info into the ray tracing pipeline
    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCInfo{};
    rayTracingPipelineCInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayTracingPipelineCInfo.stageCount = static_cast<uint32_t>(stages.size());
    rayTracingPipelineCInfo.pStages = stages.data();
    rayTracingPipelineCInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
    rayTracingPipelineCInfo.pGroups = shaderGroups.data();
    rayTracingPipelineCInfo.maxPipelineRayRecursionDepth = 1;
    rayTracingPipelineCInfo.layout = pipeLineLayout;

    vkCreateRayTracingPipelinesKHR(device, {}, {}, 1, & rayTracingPipelineCInfo, nullptr, &rayTracingPipeline);

    // After all the processing with the modules is over, destroy them
    for (auto& s : stages) {
        vkDestroyShaderModule(device, s.module, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE THE DEPTH RESOURCES
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RayTrace::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat RayTrace::findDepthFormat() {
    return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat RayTrace::findSupportedFormat(const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : potentialFormats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(GPU, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    std::_Xruntime_error("Failed to find a supported format!");
}

void RayTrace::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    createImage(SWChainExtent.width, SWChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE COMMAND POOL AND BUFFER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RayTrace::createCommandPool() {
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

void RayTrace::createCommandBuffers() {
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

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

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
        RPBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        RPBeginInfo.pClearValues = clearValues.data();

        // Finally, begin the render pass
        vkCmdBeginRenderPass(commandBuffers[i], &RPBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind the raytracing pipeline, and instruct it to draw the triangle
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, rayTracingPipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(loadedModels[0].indices.size()), 1, 0, 0, 0);

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

void RayTrace::createSemaphores(const int maxFramesInFlight) {
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

void RayTrace::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate the image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void RayTrace::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
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

void RayTrace::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    if (format == VK_FORMAT_D32_SFLOAT) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
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
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        std::_Xinvalid_argument("Unsupported layout transition!");
    }

   vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void RayTrace::createTextureImage() {
    int textureWidth, textureHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &textureWidth, &textureHeight, &texChannels, STBI_rgb_alpha);
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

VkImageView RayTrace::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo imageViewCInfo{};
    imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCInfo.image = image;
    imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCInfo.format = format;
    imageViewCInfo.subresourceRange.aspectMask = aspectFlags;
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

void RayTrace::createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void RayTrace::createTextureImageSampler() {
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

void RayTrace::cleanupSWChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (size_t i = 0; i < SWChainFrameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, SWChainFrameBuffers[i], nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(device, rayTracingPipeline, nullptr);
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
RAYTRACING METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RayTrace::initializeRT() {
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &physicalDeviceRTProperties;
    vkGetPhysicalDeviceProperties2(GPU, &deviceProps2);
}

VkDeviceAddress RayTrace::getDeviceAddress(VkBuffer buffer) {
    VkBufferDeviceAddressInfo addressInfo;
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.pNext = nullptr;
    addressInfo.buffer = buffer;
    return vkGetBufferDeviceAddressKHR(device, &addressInfo);
}

RayTrace::BLASInput RayTrace::BLASObjectToGeometry(Model model) {
    VkDeviceAddress vertexBufferAddress = getDeviceAddress(vertexBuffer);
    VkDeviceAddress indexBufferAddress = getDeviceAddress(indexBuffer);

    // Assuming trianlges, divide indices by 3
    uint32_t maxNumPrimitives = model.totalIndices / 3;

    VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress = vertexBufferAddress;
    triangles.vertexStride = sizeof(Vertex);

    triangles.indexType = VK_INDEX_TYPE_UINT32;
    triangles.indexData.deviceAddress = indexBufferAddress;
    triangles.maxVertex = model.totalVertices;

    VkAccelerationStructureGeometryKHR makeGeometry{};
    makeGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    makeGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    makeGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    makeGeometry.geometry.triangles = triangles;

    VkAccelerationStructureBuildRangeInfoKHR offset;
    offset.firstVertex = 0;
    offset.primitiveCount = maxNumPrimitives;
    offset.primitiveOffset = 0;
    offset.transformOffset = 0;

    BLASInput input;
    input.geoData.emplace_back(makeGeometry);
    input.offsetData.emplace_back(offset);

    return input;
}

bool hasFlag(VkBuildAccelerationStructureFlagsKHR flags, VkBuildAccelerationStructureFlagBitsKHR bitFlag) {
    return (flags && bitFlag != 0);
}

void RayTrace::CMDCreateBLAS(std::vector<uint32_t> indices, VkDeviceAddress scratchBufferAddress) {
    if (queryPool) {
        vkResetQueryPool(device, queryPool, 0, static_cast<uint32_t>(indices.size()));
    }
    uint32_t queryCount = 0;

    for (const auto& index : indices) {
        VkAccelerationStructureCreateInfoKHR accelStructureCInfo{};
        accelStructureCInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelStructureCInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelStructureCInfo.size = buildAS[index].sizeInfo.accelerationStructureSize;

        createBuffer(accelStructureCInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tempBuffer, tempBufferMemory);

        accelStructureCInfo.buffer = tempBuffer;
        vkCreateAccelerationStructureKHR(device, &accelStructureCInfo, nullptr, &buildAS[index].accelStructure);

        buildAS[index].buildInfo.dstAccelerationStructure = buildAS[index].accelStructure;
        buildAS[index].buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        buildAS[index].buildInfo.scratchData.deviceAddress = scratchBufferAddress;

        VkCommandBuffer cmdBuff = beginSingleTimeCommands();
        vkCmdBuildAccelerationStructuresKHR(cmdBuff, 1, &buildAS[index].buildInfo, &buildAS[index].rangeInfo);
        endSingleTimeCommands(cmdBuff);

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        VkCommandBuffer cmdBuff2 = beginSingleTimeCommands();
        vkCmdPipelineBarrier(cmdBuff2, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        endSingleTimeCommands(cmdBuff2);

        if (queryPool) {
            VkCommandBuffer cmdBuff1 = beginSingleTimeCommands();
            vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuff1, 1, &buildAS[index].buildInfo.dstAccelerationStructure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCount++);
            endSingleTimeCommands(cmdBuff1);
        }
    }
}


void RayTrace::CMDCompactBLAS(std::vector<uint32_t> indices) {
    uint32_t queryCount = 0;

    std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
    vkGetQueryPoolResults(device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize), compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

    for (auto index : indices) {
        buildAS[index].prevStructure = buildAS[index].buildInfo.dstAccelerationStructure;
        buildAS[index].accelStructure = VK_NULL_HANDLE;
        buildAS[index].sizeInfo.accelerationStructureSize = compactSizes[queryCount++];

        // Creating a compact version of the AS
        VkAccelerationStructureCreateInfoKHR accelStructureCInfo{};
        accelStructureCInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelStructureCInfo.size = buildAS[index].sizeInfo.accelerationStructureSize;
        accelStructureCInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        createBuffer(accelStructureCInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tempBuffer2, tempBufferMemory2);

        accelStructureCInfo.buffer = tempBuffer;
        vkCreateAccelerationStructureKHR(device, &accelStructureCInfo, nullptr, &buildAS[index].accelStructure);

        VkCopyAccelerationStructureInfoKHR copyInfo{};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
        copyInfo.src = buildAS[index].buildInfo.dstAccelerationStructure;
        copyInfo.dst = buildAS[index].accelStructure;
        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        VkCommandBuffer cmdBuffer1 = beginSingleTimeCommands();
        vkCmdCopyAccelerationStructureKHR(cmdBuffer1, &copyInfo);
        endSingleTimeCommands(cmdBuffer1);
        vkDestroyBuffer(device, tempBuffer, nullptr);
        vkDestroyBuffer(device, tempBuffer2, nullptr);
    }
}

void RayTrace::buildBlas(const std::vector<BLASInput>& input, VkBuildAccelerationStructureFlagsKHR flags) {
    uint32_t numBlas = static_cast<uint32_t>(input.size());
    VkDeviceSize asTotalSize{ 0 };
    uint32_t numCompactions{ 0 };
    VkDeviceSize maxScratchSize{ 0 };

    buildAS.resize(numBlas);
    for (int i = 0; i < numBlas; i++) {
        buildAS[i].sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        buildAS[i].buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildAS[i].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildAS[i].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildAS[i].buildInfo.flags = input[i].flags | flags;
        buildAS[i].buildInfo.geometryCount = static_cast<uint32_t>(input[i].geoData.size());
        buildAS[i].buildInfo.pGeometries = input[i].geoData.data();

        buildAS[i].rangeInfo = input[i].offsetData.data();

        std::vector<uint32_t> maxPrimitiveCount(input[i].offsetData.size());
        for (auto j = 0; j < input[i].offsetData.size(); j++) {
            maxPrimitiveCount[j] = input[i].offsetData[j].primitiveCount;
            vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildAS[i].buildInfo, maxPrimitiveCount.data(), &buildAS[i].sizeInfo);
        }

        asTotalSize += buildAS[i].sizeInfo.accelerationStructureSize;
        maxScratchSize = std::max(maxScratchSize, buildAS[i].sizeInfo.buildScratchSize);
        numCompactions += hasFlag(buildAS[i].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
    }

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;

    createBuffer(maxScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBuffer, scratchBufferMemory);
    VkBufferDeviceAddressInfo scratchBufferInfo{};
    scratchBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchBufferInfo.buffer = scratchBuffer;
    VkDeviceAddress scratchBufferAddress = vkGetBufferDeviceAddressKHR(device, &scratchBufferInfo);

    if (numCompactions > 0) {
        VkQueryPoolCreateInfo queryPoolCInfo{};
        queryPoolCInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCInfo.queryCount = numCompactions;
        queryPoolCInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        vkCreateQueryPool(device, &queryPoolCInfo, nullptr, &queryPool);
        vkDeviceWaitIdle(device);
    }

    vkDeviceWaitIdle(device);

    std::vector<uint32_t> indices;
    VkDeviceSize batchSize{ 0 };
    VkDeviceSize batchLimit{ 256'000'000 };

    for (int i = 0; i < numBlas; i++) {
        indices.push_back(i);
        batchSize += buildAS[i].sizeInfo.accelerationStructureSize;
        if (batchSize >= batchLimit || i == numBlas - 1) {
            CMDCreateBLAS(indices, scratchBufferAddress);

            if (queryPool) {
                CMDCompactBLAS(indices);
            }

            batchSize = 0;
            indices.clear();
        }
    }
    for (BuildAccelerationStructure& b : buildAS)
    {
        bottomLevelAccelerationStructures.emplace_back(b.accelStructure);
    }

    vkDestroyQueryPool(device, queryPool, nullptr);
    vkDestroyBuffer(device, scratchBuffer, nullptr);
}

void RayTrace::createBottomLevelAS() {
    std::vector<BLASInput> BLASInputList;
    BLASInputList.reserve(numModels);
    for (int i = 0; i < numModels; i++) {
        BLASInputList.emplace_back(BLASObjectToGeometry(loadedModels[i]));
    }

    buildBlas(BLASInputList, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void RayTrace::CMDCreateTLAS(uint32_t numInstances, VkDeviceAddress instBufferAddress, VkBuffer scratchBuffer, VkBuildAccelerationStructureFlagsKHR flags, bool update, bool motion) {
    VkAccelerationStructureGeometryInstancesDataKHR vulkanInstances{};
    vulkanInstances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    vulkanInstances.data.deviceAddress = instBufferAddress;

    VkAccelerationStructureGeometryKHR topAccelStructGeo{};
    topAccelStructGeo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    topAccelStructGeo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topAccelStructGeo.geometry.instances = vulkanInstances;

    VkAccelerationStructureBuildGeometryInfoKHR topAccelStructBuildInfo{};
    topAccelStructBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    topAccelStructBuildInfo.flags = flags;
    topAccelStructBuildInfo.geometryCount = 1;
    topAccelStructBuildInfo.pGeometries = &topAccelStructGeo;
    topAccelStructBuildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    topAccelStructBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    topAccelStructBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    VkAccelerationStructureBuildSizesInfoKHR topAccelStructSizeInfo{};
    topAccelStructSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &topAccelStructBuildInfo, &numInstances, &topAccelStructSizeInfo);

    VkAccelerationStructureCreateInfoKHR topAccelStructCInfo{};
    topAccelStructCInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    topAccelStructCInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    topAccelStructCInfo.size = topAccelStructSizeInfo.accelerationStructureSize;

    createBuffer(topAccelStructCInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tempBuffer3, tempBufferMemory3);
    topAccelStructCInfo.buffer = tempBuffer3;

    VkResult res = vkCreateAccelerationStructureKHR(device, &topAccelStructCInfo, nullptr, &topLevelAccelStructure);

    VkDeviceMemory scratchBufferMemory;
    createBuffer(topAccelStructSizeInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBuffer, scratchBufferMemory);
    VkBufferDeviceAddressInfo scratchBufferInfo{};
    scratchBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchBufferInfo.buffer = scratchBuffer;
    VkDeviceAddress scratchBufferAddress = vkGetBufferDeviceAddress(device, &scratchBufferInfo);

    topAccelStructBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    topAccelStructBuildInfo.dstAccelerationStructure = topLevelAccelStructure;
    topAccelStructBuildInfo.scratchData.deviceAddress = scratchBufferAddress;

    VkAccelerationStructureBuildRangeInfoKHR TLASOffsetInfo{};
    TLASOffsetInfo.firstVertex = numInstances;
    TLASOffsetInfo.primitiveCount, TLASOffsetInfo.primitiveOffset, TLASOffsetInfo.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* pTLASBuildOffsetInfo = &TLASOffsetInfo;

    VkCommandBuffer cmdBuff1 = beginSingleTimeCommands();
    vkCmdBuildAccelerationStructuresKHR(cmdBuff1, 1, &topAccelStructBuildInfo, &pTLASBuildOffsetInfo);
    endSingleTimeCommands(cmdBuff1);
}

void RayTrace::buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& TLASInstances, VkBuildAccelerationStructureFlagsKHR flags, bool update = false) {
    uint32_t numInstances = static_cast<uint32_t>(instances.size());

    VkBuffer instancesBuffer;
    VkDeviceMemory instancesBufferMemory;
    createBuffer(TLASInstances.size(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, instancesBuffer, instancesBufferMemory);
    VkBufferDeviceAddressInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferInfo.buffer = instancesBuffer;
    VkDeviceAddress instBufferAddress = vkGetBufferDeviceAddress(device, &bufferInfo);

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    VkCommandBuffer cmdBuff = beginSingleTimeCommands();
    vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    endSingleTimeCommands(cmdBuff);

    VkBuffer scratchBuffer = VK_NULL_HANDLE;
    bool motion = false;
    CMDCreateTLAS(numInstances, instBufferAddress, scratchBuffer, flags, update, motion);
    vkDestroyBuffer(device, scratchBuffer, nullptr);
    vkDestroyBuffer(device, instancesBuffer, nullptr);
}

void RayTrace::createTopLevelAS() {
    std::vector<VkAccelerationStructureInstanceKHR> TLAS;
    TLAS.reserve(instances.size());
    for (const OBJInstance& inst : instances) {
        VkAccelerationStructureInstanceKHR rayInstance{};

        glm::mat4 temporaryMatrix = glm::transpose(inst.transform);
        VkTransformMatrixKHR otherMatrix;
        memcpy(&otherMatrix, &temporaryMatrix, sizeof(VkTransformMatrixKHR));

        rayInstance.transform = otherMatrix;
        rayInstance.instanceCustomIndex = inst.index;

        VkAccelerationStructureDeviceAddressInfoKHR BLASAddressInfo{};
        BLASAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        BLASAddressInfo.accelerationStructure = buildAS[inst.index].accelStructure;

        rayInstance.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(device, &BLASAddressInfo);
        rayInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        rayInstance.mask = 0xFF;
        rayInstance.instanceShaderBindingTableRecordOffset = 0;
        TLAS.emplace_back(rayInstance);
    }
    buildTlas(TLAS, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void RayTrace::createStorageImage() {
    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkImageCreateInfo storageImageCInfo{};
    storageImageCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    storageImageCInfo.imageType = VK_IMAGE_TYPE_2D;
    storageImageCInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    storageImageCInfo.extent.width = SWChainExtent.width;
    storageImageCInfo.extent.height = SWChainExtent.height;
    storageImageCInfo.extent.depth = 1;
    storageImageCInfo.mipLevels = 1;
    storageImageCInfo.arrayLayers = 1;
    storageImageCInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    storageImageCInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    storageImageCInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    storageImageCInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(device, &storageImageCInfo, nullptr, &storageImage);

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, storageImage, &memoryRequirements);
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &storageImageMemory);
    vkBindImageMemory(device, storageImage, storageImageMemory, 0);

    VkImageViewCreateInfo storageImageViewCInfo{};
    storageImageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    storageImageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    storageImageViewCInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    storageImageViewCInfo.subresourceRange = {};
    storageImageViewCInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    storageImageViewCInfo.subresourceRange.baseMipLevel = 0;
    storageImageViewCInfo.subresourceRange.levelCount = 1;
    storageImageViewCInfo.subresourceRange.baseArrayLayer = 0;
    storageImageViewCInfo.subresourceRange.layerCount = 1;
    storageImageViewCInfo.image = storageImage;
    vkCreateImageView(device, &storageImageViewCInfo, nullptr, &storageImageView);

    VkCommandBuffer cmdbuff = beginSingleTimeCommands();
    setImageLayout(cmdbuff, storageImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    endSingleTimeCommands(cmdbuff);
}

void RayTrace::setImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier imageBarrier;
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = image;

    switch (oldLayout) {
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        imageBarrier.srcAccessMask =
            VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    }

    switch (newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageBarrier.dstAccessMask |=
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imageBarrier.srcAccessMask =
            VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    }

    VkPipelineStageFlagBits srcFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlagBits dstFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    vkCmdPipelineBarrier(cmdBuffer, srcFlags, dstFlags, 0, 0, NULL, 0, NULL, 1, &imageBarrier);
}

void RayTrace::recreateSwapChain(SDL_Window* window) {
    vkDeviceWaitIdle(device);

    cleanupSWChain();

    createSWChain(window);
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFrameBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createRTDescriptorSet();
    createCommandBuffers();
}