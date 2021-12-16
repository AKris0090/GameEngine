#pragma once

#include "vulkan/vulkan.h"
#include "SDL.h"
#include <optional>
#include <vector>
#include <cstdio>

class VulkanRenderer {

private:
public:

	// Instance variables and extension support
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debugMessenger;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> SWChainImages;
	VkFormat SWChainImageFormat;
	VkExtent2D SWChainExtent;
	std::vector<VkImageView> SWChainImageViews;

	VkPhysicalDevice GPU;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	struct SWChainSuppDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		VkSurfaceFormatKHR chooseSwSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwPresMode(const std::vector<VkPresentModeKHR>& availablePresModes);
		VkExtent2D chooseSwExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
	};

#ifdef NDEBUG
	const bool enableValLayers = false;
#else
	const bool enableValLayers = true;
#endif
	

	VkInstance createVulkanInstance(SDL_Window* window, const char* appName);
	bool checkValLayerSupport();
	void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void createSurface(SDL_Window* window);
	bool checkExtSupport(VkPhysicalDevice physicalDevice);
	SWChainSuppDetails getDetails(VkPhysicalDevice physicalDevice);
	void createSWChain(SDL_Window* window);
	bool isSuitable(VkPhysicalDevice physicalDevice);
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createImageViews();


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