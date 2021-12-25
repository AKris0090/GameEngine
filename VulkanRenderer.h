#pragma once

#include "vulkan/vulkan.h"
#include "SDL.h"
#include <optional>
#include <vector>
#include <cstdio>
#include <string>
#include "glm-0.9.6.3/glm.hpp"
#include <array>

#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

class VulkanRenderer {

private:
public:

	// Extension and validation arrays
	const std::vector<const char*> extNames{};
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	const std::vector<const char*> deviceExts = {
		"VK_KHR_swapchain",
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
	};


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

	VkDescriptorSetLayout descriptorSetLayout;

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
	std::vector<VkSemaphore> imageAcquiredSema;
	std::vector<VkSemaphore> renderedSema;

	// Handle for the in-flight-fence
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	// IF NEEDED, HANDLE WINDOW MINIMIZATION AND RESIZE

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkMemoryRequirements memRequirements;
	VkPhysicalDeviceMemoryProperties memProperties;
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
			return attributeDescriptions;
		}
	};

	const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
	};

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};

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

	void createDescriptorSetLayout();

	// Create the graphics pipeline
	void createGraphicsPipeline();
	// Creating the all-important frame buffer
	void createFrameBuffer();
	// You have to first record all the operations to perform, so we need a command pool
	void createCommandPool();

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createTextureImage();
	VkImageView createImageView(VkImage image, VkFormat format);
	void createTextureImageView();
	void createTextureImageSampler();

	// Create a list of command buffer objects
	void createCommandBuffers();

	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	// Create the semaphores, signaling objects to allow asynchronous tasks to happen at the same time
	void createSemaphores(const int maxFramesInFlight);

	// Additional swap chain methods
	void cleanupSWChain();
	void recreateSwapChain(SDL_Window* window);

	// Helper methods for the graphics pipeline
	static std::vector<char> readFile(const std::string& fileName);
	VkShaderModule createShaderModule(const std::vector<char>& binary);
	
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	// Ray tracing methods and handles
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRTProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	void initializeRT();


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