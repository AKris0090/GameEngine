#include "Display.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <cstdio>
#include "vulkan/vulkan.h"
#include "VulkanRenderer.h"

#define SDL_MAIN_HANDLED

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

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

void Display::drawNewFrame(VulkanRenderer v) {
    // Acquire an image from the swap chain, execute the command buffer with the image attached in the framebuffer, and return to swap chain as ready to present
    uint32_t imageIndex;
    // Disable the timeout with UINT64_MAX
    vkAcquireNextImageKHR(v.device, v.swapChain, UINT64_MAX, v.imageAccquiredSema, VK_NULL_HANDLE, &imageIndex);

    // Submit the command buffer with the semaphore
    VkSubmitInfo queueSubmitInfo{};
    queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { v.imageAccquiredSema };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    queueSubmitInfo.waitSemaphoreCount = 1;
    queueSubmitInfo.pWaitSemaphores = waitSemaphores;
    queueSubmitInfo.pWaitDstStageMask = waitStages;

    // Specify the command buffers to actually submit for execution
    queueSubmitInfo.commandBufferCount = 1;
    queueSubmitInfo.pCommandBuffers = &v.commandBuffers[imageIndex];

    // Specify which semaphores to signal once command buffers have finished execution
    VkSemaphore signaledSemaphores[] = { v.renderedSema };
    queueSubmitInfo.signalSemaphoreCount = 1;
    queueSubmitInfo.pSignalSemaphores = signaledSemaphores;

    // Finally, submit the queue info
    if (vkQueueSubmit(v.graphicsQueue, 1, &queueSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to submit the draw command buffer to the graphics queue!");
    }

    // Present the frame from the queue
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Specify the semaphores to wait on before presentation happens
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signaledSemaphores;

    // Specify the swap chains to present images and the image index for each chain
    VkSwapchainKHR swapChains[] = { v.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    // Check if the presentation was successful
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(v.presentQueue, &presentInfo);
}