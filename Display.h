#pragma once

#include "SDL.h"
#include "VulkanRenderer.h"
#include "RayTrace.h"


class Display {

public:
	size_t currentFrame = 0;

	SDL_Window* initDisplay(const char* appName);
	void drawNewFrame(VulkanRenderer v, int maxFramesInFlight, std::vector<VkFence> inFlightFences, std::vector<VkFence> imagesInFlight);
	void drawNewFrameRT(RayTrace rt, int maxFramesInFlight, std::vector<VkFence> inFlightFences, std::vector<VkFence> imagesInFlight);
	void updateUniformBuffer(uint32_t currentImageIndex, VulkanRenderer vkR);
	void updateUniformBufferRT(uint32_t currentImageIndex, RayTrace rt);
};