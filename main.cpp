#include "SDL.h"
#include "Display.h"
#include "VulkanRenderer.h"
#include <vector>
#include "glm-0.9.6.3/glm.hpp"
#include "glm-0.9.6.3/gtc/matrix_transform.hpp"
#include <cstring>
#include "RayTrace.h"

VulkanRenderer vkR;
SDL_Window* displayWindow;
bool yesTexture = true;
const int MAX_FRAMES_IN_FLIGHT = 2;

#define VOLK_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "volk-master/volk.h"
#include <volk.h>

void cleanup() {
    vkR.cleanupSWChain();

    //for (auto& as : vkR.buildAS) {
    //    as.cleanupAS(vkR.device);
    //}

    vkDestroyCommandPool(vkR.device, vkR.commandPool, nullptr);

    vkDestroyImageView(vkR.device, vkR.depthImageView, nullptr);
    vkDestroyImage(vkR.device, vkR.depthImage, nullptr);
    vkFreeMemory(vkR.device, vkR.depthImageMemory, nullptr);

    vkDestroySampler(vkR.device, vkR.textureSampler, nullptr);
    vkDestroyImageView(vkR.device, vkR.textureImageView, nullptr);

    vkDestroyImage(vkR.device, vkR.textureImage, nullptr);
    vkFreeMemory(vkR.device, vkR.textureImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(vkR.device, vkR.descriptorSetLayout, nullptr);

    vkDestroyBuffer(vkR.device, vkR.indexBuffer, nullptr);
    vkFreeMemory(vkR.device, vkR.indexBufferMemory, nullptr);

    vkDestroyBuffer(vkR.device, vkR.vertexBuffer, nullptr);
    vkFreeMemory(vkR.device, vkR.vertexBufferMemory, nullptr);

    for (size_t i = 0; i < vkR.SWChainImages.size(); i++) {
        vkDestroyBuffer(vkR.device, vkR.uniformBuffers[i], nullptr);
        vkFreeMemory(vkR.device, vkR.uniformBuffersMemory[i], nullptr);
    }

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
    glm::mat4 translationMatrix{ 1.0f };

    volkInitialize();

    vkR.instance = vkR.createVulkanInstance(displayWindow, "Vulkan Game Engine");

    volkLoadInstance(vkR.instance);

    if (vkR.enableValLayers) {
        vkR.setupDebugMessenger(vkR.instance, vkR.debugMessenger);
    }

    vkR.createSurface(displayWindow);

    vkR.pickPhysicalDevice();

    vkR.createLogicalDevice();

    volkLoadDevice(vkR.device);

    vkR.createSWChain(displayWindow);

    vkR.createImageViews();

    vkR.createRenderPass();

    vkR.createDescriptorSetLayout();

    vkR.createGraphicsPipeline();

    vkR.createCommandPool();

    vkR.createDepthResources();

    vkR.createFrameBuffer();

    if (yesTexture) {

        vkR.createTextureImage();

        vkR.createTextureImageView();

        vkR.createTextureImageSampler();
    }

    vkR.loadModel(translationMatrix);

    vkR.createVertexBuffer();

    vkR.createIndexBuffer();

    vkR.createUniformBuffers();

    vkR.createDescriptorPool();

    vkR.createDescriptorSets();

    vkR.createCommandBuffers();

    vkR.createSemaphores(MAX_FRAMES_IN_FLIGHT);
}

void initRayTrace() {
    RayTrace rt;

    glm::mat4 translationMatrix{ 1.0f };

    RayTrace::PushConstantRay m_pcRay{};

    volkInitialize();

    rt.instance = rt.createVulkanInstance(displayWindow, "Ray Tracing Game Engine");

    volkLoadInstance(rt.instance);

    if (rt.enableValLayers) {
        rt.setupDebugMessenger(rt.instance, rt.debugMessenger);
    }

    rt.createSurface(displayWindow);

    rt.pickPhysicalDevice();

    rt.createLogicalDevice();

    volkLoadDevice(rt.device);

    rt.createCommandPool();

    rt.createSWChain(displayWindow);

    rt.loadModel(translationMatrix);

    rt.createVertexBuffer();

    rt.createIndexBuffer();

    rt.createUniformBuffers();

    rt.createBottomLevelAS();
    
    rt.createTopLevelAS();

    rt.createImageViews();

    rt.createStorageImage();

    rt.createTextureImage();

    rt.createTextureImageView();

    rt.createTextureImageSampler();

    rt.createRenderPass();

    rt.createDescriptorPool();

    rt.createDescriptorSetLayout();

    rt.createRTDescriptorSet();

    rt.createRtPipeline();

    rt.createDepthResources();

    rt.createFrameBuffer();

    rt.createTextureImage();

    rt.createTextureImageView();

    rt.createTextureImageSampler();

    rt.createCommandBuffers();

    rt.createSemaphores(MAX_FRAMES_IN_FLIGHT);
}

int main(int argc, char** arcgv) {

    Display d;
    displayWindow = d.initDisplay("Vulkan Game Engine");

    initRayTrace();

    executeVulkanSDLLoop(d);

    return 0;
}