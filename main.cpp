#include "SDL.h"
#include "Display.h"
#include "VulkanRenderer.h"
#include <vector>
#include <cstring>

VulkanRenderer vkR;
SDL_Window* displayWindow;

void cleanup(VulkanRenderer v) {
    vkDestroyPipeline(v.device, v.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(v.device, v.pipeLineLayout, nullptr);

    vkDestroyPipelineLayout(v.device, v.pipeLineLayout, nullptr);
    vkDestroyRenderPass(v.device, v.renderPass, nullptr);

    for (auto imageView : v.SWChainImageViews) {
        vkDestroyImageView(v.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(v.device, v.swapChain, nullptr);
    vkDestroyDevice(v.device, nullptr);

    if (v.enableValLayers) {
        v.DestroyDebugUtilsMessengerEXT(v.instance, v.debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(v.instance, v.surface, nullptr);
    vkDestroyInstance(v.instance, nullptr);
}

void executeVulkanSDLLoop(Display d, VulkanRenderer v) {
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
    }
    // Cleanup after looping before exiting program
    cleanup(v);
    SDL_DestroyWindow(displayWindow);
    SDL_Quit();
}

void initVulkan(VulkanRenderer vkR) {
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
}

int main(int argc, char** arcgv) {

    Display d;
    displayWindow = d.initDisplay("Vulkan Game Engine");

    initVulkan(vkR);

    executeVulkanSDLLoop(d, vkR);

    return 0;
}