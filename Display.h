#pragma once

#include "SDL.h"
#include "VulkanRenderer.h"

class Display {

public:
	SDL_Window* initDisplay(const char* appName);
	void drawNewFrame(VulkanRenderer v);
};