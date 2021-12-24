#include "Display.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <cstdio>
#include "vulkan/vulkan.h"
#include "VulkanRenderer.h"
#include "glm-0.9.6.3/glm.hpp"
#include "glm-0.9.6.3/gtc/matrix_transform.hpp"
#include <chrono>

#define GLM_FORCE_RADIANS
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

void Display::updateUniformBuffer(uint32_t currentImage, VulkanRenderer vkR) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    VulkanRenderer::UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), vkR.SWChainExtent.width / (float)vkR.SWChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(vkR.device, vkR.uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vkR.device, vkR.uniformBuffersMemory[currentImage]);
}

void Display::drawNewFrame(VulkanRenderer v, int maxFramesInFlight, std::vector<VkFence> inFlightFences, std::vector<VkFence> imagesInFlight) {
    // Wait for the frame to be finished, with the fences
    vkWaitForFences(v.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swap chain, execute the command buffer with the image attached in the framebuffer, and return to swap chain as ready to present
    uint32_t imageIndex;
    // Disable the timeout with UINT64_MAX
    VkResult res1 = vkAcquireNextImageKHR(v.device, v.swapChain, UINT64_MAX, v.imageAcquiredSema[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (res1 == VK_ERROR_OUT_OF_DATE_KHR) {
        v.recreateSwapChain(window);
        return;
    }
    else if (res1 != VK_SUCCESS && res1 != VK_SUBOPTIMAL_KHR) {
        std::_Xruntime_error("Failed to acquire a swap chain image!");
    }

    updateUniformBuffer(imageIndex, v);

    // Check to make sure previous frame isnt using the image
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(v.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    // Now mark the new image as being used by the frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // Submit the command buffer with the semaphore
    VkSubmitInfo queueSubmitInfo{};
    queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { v.imageAcquiredSema[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    queueSubmitInfo.waitSemaphoreCount = 1;
    queueSubmitInfo.pWaitSemaphores = waitSemaphores;
    queueSubmitInfo.pWaitDstStageMask = waitStages;

    // Specify the command buffers to actually submit for execution
    queueSubmitInfo.commandBufferCount = 1;
    queueSubmitInfo.pCommandBuffers = &v.commandBuffers[imageIndex];

    // Specify which semaphores to signal once command buffers have finished execution
    VkSemaphore signaledSemaphores[] = { v.renderedSema[currentFrame] };
    queueSubmitInfo.signalSemaphoreCount = 1;
    queueSubmitInfo.pSignalSemaphores = signaledSemaphores;

    vkResetFences(v.device, 1, &inFlightFences[currentFrame]);

    // Finally, submit the queue info
    if (vkQueueSubmit(v.graphicsQueue, 1, &queueSubmitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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

    VkResult res2 = vkQueuePresentKHR(v.presentQueue, &presentInfo);
    if (res2 == VK_ERROR_OUT_OF_DATE_KHR || res2 == VK_SUBOPTIMAL_KHR) {
        v.recreateSwapChain(window);
    }
    else if (res2 != VK_SUCCESS) {
        std::_Xruntime_error("Failed to present a swap chain image!");
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}