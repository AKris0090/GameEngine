#include "Display.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <cstdio>

#define SDL_MAIN_HANDLED

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

SDL_Window* window;
SDL_Renderer* renderer;

SDL_Window* Display::initDisplay(const char* appName) {

    // Startup the video feed
    SDL_Init(SDL_INIT_VIDEO);

    // Create the SDL Window and open
    window = SDL_CreateWindow(appName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN);

    // Create the renderer for the window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    return window;
}