#pragma once

#include "SDL.h"
#include "VulkanRenderer.h"

class Display {

public:
	size_t currentFrame = 0;

	SDL_Window* initDisplay(const char* appName);
	void drawNewFrame(VulkanRenderer v, int maxFramesInFlight, std::vector<VkFence> inFlightFences, std::vector<VkFence> imagesInFlight);
};