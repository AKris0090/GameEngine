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

#ifdef NDEBUG
const bool enableValLayers = false;
#else
const bool enableValLayers = true;
#endif

// Extension and validation arrays
const std::vector<const char*> extNames{};
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExts = {
    "VK_KHR_swapchain"
};


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

    return (indices.isComplete() && extsSupported && isSWChainAdequate);
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

    // Create the logical device, filling in with the create info structs
    VkDeviceCreateInfo cInfo{};
    cInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    cInfo.queueCreateInfoCount = static_cast<uint32_t>(queuecInfos.size());
    cInfo.pQueueCreateInfos = queuecInfos.data();

    cInfo.pEnabledFeatures = &gpuFeatures;

    // Set enabledLayerCount and ppEnabledLayerNames fields to be compatible with older implementations of Vulkan
    cInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
    cInfo.ppEnabledExtensionNames = deviceExts.data();

    // If validation layers are enabled, then fill create info struct with size and name information
    if (enableValLayers) {
        cInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        cInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        cInfo.enabledLayerCount = 0;
    }

    // Instantiate
    if (vkCreateDevice(GPU, &cInfo, nullptr, &device) != VK_SUCCESS) {
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

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        VkImageViewCreateInfo imageViewCInfo{};

        // Fill the create info struct with basic information regarding the image view
        imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCInfo.image = SWChainImages[i];
        imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCInfo.format = SWChainImageFormat;

        // Components allow for swizzling the color channels, like mapping to monochrome or adding filters
        imageViewCInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Subresource Range is the image's intended purpose, and we define which parts of the image we need to access
        imageViewCInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCInfo.subresourceRange.baseMipLevel = 0;
        imageViewCInfo.subresourceRange.levelCount = 1;
        imageViewCInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCInfo.subresourceRange.layerCount = 1;

        // Create the image view
        VkResult res = vkCreateImageView(device, &imageViewCInfo, nullptr, &SWChainImageViews[i]);

        if (res != VK_SUCCESS) {
            std::_Xruntime_error("Failed to create image views!");
        }
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

    if (vkCreateRenderPass(device, &renderPassCInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create render pass!");
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
    vertexInputCInfo.vertexBindingDescriptionCount = 0;
    vertexInputCInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCInfo.pVertexAttributeDescriptions = nullptr;

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
    rasterizerCInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

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
    pipeLineLayoutCInfo.setLayoutCount = 0;
    pipeLineLayoutCInfo.pSetLayouts = nullptr;
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