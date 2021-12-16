#pragma once

#include "SDL.h"

class Display {

public:
	SDL_Window* initDisplay(const char* appName);
};