#pragma once

#include "vulkan/vulkan.h"
#include "SDL.h"
#include <optional>
#include <vector>
#include <cstdio>
#include <string>

class VulkanRenderer {

private:
public:

	// Handles for all variables, made public so they can be accessed by main
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debugMessenger;

	// Swap chain handles
	VkSwapchainKHR swapChain;
	std::vector<VkImage> SWChainImages;
	VkFormat SWChainImageFormat;
	VkExtent2D SWChainExtent;
	std::vector<VkImageView> SWChainImageViews;

	// Device and queue handles
	VkPhysicalDevice GPU;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// Color Blending
	bool colorBlendEnable = true;

	// Pipeline Layout for "gloabls" to change shaders
	VkPipelineLayout pipeLineLayout;

	// Render pass handles
	VkRenderPass renderPass;

	// The graphics pipeline handle
	VkPipeline graphicsPipeline;

	// Handle to hold the frame buffers
	std::vector<VkFramebuffer> SWChainFrameBuffers;

	// Command Buffers - Command pool and command buffer handles
	VkCommandPool commandPool;

	// Handle for the list of command buffers
	std::vector<VkCommandBuffer> commandBuffers;

	// Handles for the two semaphores - one for if the image is available and the other to present the image
	VkSemaphore imageAccquiredSema;
	VkSemaphore renderedSema;



	// Swap chain support details struct - holds information to create the swapchain
	struct SWChainSuppDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		VkSurfaceFormatKHR chooseSwSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwPresMode(const std::vector<VkPresentModeKHR>& availablePresModes);
		VkExtent2D chooseSwExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
	};

	// If not debugging, enable validation layers
#ifdef NDEBUG
	const bool enableValLayers = false;
#else
	const bool enableValLayers = true;
#endif
	
	// Create the vulkan instance
	VkInstance createVulkanInstance(SDL_Window* window, const char* appName);
	// Check if the validation layers requested are supported
	bool checkValLayerSupport();
	// Debug messenger methods - create, populate, and destroy the debug messenger
	void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	// Create the SDL surface using Vulkan
	void createSurface(SDL_Window* window);
	// Check if the extensions requested are supported by the physical device - GPU
	bool checkExtSupport(VkPhysicalDevice physicalDevice);
	// Get the details for the swap chain using information from the physical device
	SWChainSuppDetails getDetails(VkPhysicalDevice physicalDevice);
	// Initialize the swap chain
	void createSWChain(SDL_Window* window);
	// While polling through the available physical devices, check each one to see if it is suitable
	bool isSuitable(VkPhysicalDevice physicalDevice);
	// Poll through the available physical devices and choose one using isSuitable()
	void pickPhysicalDevice();
	// Using the physical device, create the logical device
	void createLogicalDevice();
	// With the swap chain, create the image views
	void createImageViews();
	// Create the render pass
	void createRenderPass();
	// Create the graphics pipeline
	void createGraphicsPipeline();
	// Creating the all-important frame buffer
	void createFrameBuffer();
	// You have to first record all the operations to perform, so we need a command pool
	void createCommandPool();
	// Create a list of command buffer objects
	void createCommandBuffers();
	// Create the semaphores, signaling objects to allow asynchronous tasks to happen at the same time
	void createSemaphores();


	// Helper methods for the graphics pipeline
	static std::vector<char> readFile(const std::string& fileName);
	VkShaderModule createShaderModule(const std::vector<char>& binary);


	// Queue family struct
	struct QueueFamilyIndices {
		// Graphics families initialization
		std::optional<uint32_t> graphicsFamily;

		// Present families initialization as well
		std::optional<uint32_t> presentFamily;

		// General check to make things a bit more conveneient
		bool isComplete() { 
			return (graphicsFamily.has_value() && presentFamily.has_value());
		}
	};

	// Find the queue families given a physical device, called in isSuitable to find if the queue families support VK_QUEUE_GRAPHICS_BIT
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice) {
		QueueFamilyIndices indices;

		// Get queue families and store in queueFamilies array
		uint32_t numQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, queueFamilies.data());

		// Find at least 1 queue family that supports VK_QUEUE_GRAPHICS_BIT
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 prSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &prSupport);

			if (prSupport) {
				indices.presentFamily = i;
			}

			// Early exit
			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}
};