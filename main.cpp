#include "SDL.h"
#include "Display.h"
#include "VulkanRenderer.h"
#include <vector>
#include <cstring>

VulkanRenderer vkR;
SDL_Window* displayWindow;
const int MAX_FRAMES_IN_FLIGHT = 2;

void cleanup() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkR.device, vkR.imageAcquiredSema[i], nullptr);
        vkDestroySemaphore(vkR.device, vkR.renderedSema[i], nullptr);
        vkDestroyFence(vkR.device, vkR.inFlightFences[i], nullptr);
    }

    for (auto frameBuffer : vkR.SWChainFrameBuffers) {
        vkDestroyFramebuffer(vkR.device, frameBuffer, nullptr);
    }

    vkDestroyPipeline(vkR.device, vkR.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkR.device, vkR.pipeLineLayout, nullptr);

    vkDestroyPipelineLayout(vkR.device, vkR.pipeLineLayout, nullptr);
    vkDestroyRenderPass(vkR.device, vkR.renderPass, nullptr);

    for (auto imageView : vkR.SWChainImageViews) {
        vkDestroyImageView(vkR.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(vkR.device, vkR.swapChain, nullptr);
    vkDestroyDevice(vkR.device, nullptr);

    if (vkR.enableValLayers) {
        vkR.DestroyDebugUtilsMessengerEXT(vkR.instance, vkR.debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(vkR.instance, vkR.surface, nullptr);
    vkDestroyInstance(vkR.instance, nullptr);
}

void executeVulkanSDLLoop(Display d) {
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            default:
                break;
            }
        }
        // Method to draw the frame
        d.drawNewFrame(vkR, MAX_FRAMES_IN_FLIGHT, vkR.inFlightFences, vkR.imagesInFlight);
    }
    // Cleanup after looping before exiting program
    cleanup();
    SDL_DestroyWindow(displayWindow);
    SDL_Quit();
}

void initVulkan() {
    vkR.instance = vkR.createVulkanInstance(displayWindow, "Vulkan Game Engine");

    if (vkR.enableValLayers) {
        vkR.setupDebugMessenger(vkR.instance, vkR.debugMessenger);
    }

    vkR.createSurface(displayWindow);

    vkR.pickPhysicalDevice();

    vkR.createLogicalDevice();

    vkR.createSWChain(displayWindow);

    vkR.createImageViews();

    vkR.createRenderPass();

    vkR.createGraphicsPipeline();

    vkR.createFrameBuffer();

    vkR.createCommandPool();

    vkR.createCommandBuffers();

    vkR.createSemaphores(MAX_FRAMES_IN_FLIGHT);
}

int main(int argc, char** arcgv) {

    Display d;
    displayWindow = d.initDisplay("Vulkan Game Engine");

    initVulkan();

    executeVulkanSDLLoop(d);

    return 0;
}