#include "SDL.h"
#include "Display.h"
#include "VulkanRenderer.h"
#include <vector>
#include "glm-0.9.6.3/glm.hpp"
#include "glm-0.9.6.3/gtc/matrix_transform.hpp"
#include <cstring>
#include "RayTrace.h"

VulkanRenderer vkR;
RayTrace rt;
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

void cleanupRT() {
    rt.cleanupSWChain();

    //for (auto& as : rt.buildAS) {
    //    as.cleanupAS(rt.device);
    //}

    vkDestroyCommandPool(rt.device, rt.commandPool, nullptr);

    vkDestroyImageView(rt.device, rt.depthImageView, nullptr);
    vkDestroyImage(rt.device, rt.depthImage, nullptr);
    vkFreeMemory(rt.device, rt.depthImageMemory, nullptr);

    vkDestroySampler(rt.device, rt.textureSampler, nullptr);
    vkDestroyImageView(rt.device, rt.textureImageView, nullptr);

    vkDestroyImage(rt.device, rt.textureImage, nullptr);
    vkFreeMemory(rt.device, rt.textureImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(rt.device, rt.descriptorSetLayout, nullptr);

    vkDestroyBuffer(rt.device, rt.indexBuffer, nullptr);
    vkFreeMemory(rt.device, rt.indexBufferMemory, nullptr);

    vkDestroyBuffer(rt.device, rt.vertexBuffer, nullptr);
    vkFreeMemory(rt.device, rt.vertexBufferMemory, nullptr);

    for (size_t i = 0; i < vkR.SWChainImages.size(); i++) {
        vkDestroyBuffer(rt.device, rt.uniformBuffers[i], nullptr);
        vkFreeMemory(rt.device, rt.uniformBuffersMemory[i], nullptr);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(rt.device, rt.imageAcquiredSema[i], nullptr);
        vkDestroySemaphore(rt.device, rt.renderedSema[i], nullptr);
        vkDestroyFence(rt.device, rt.inFlightFences[i], nullptr);
    }

    for (auto frameBuffer : rt.SWChainFrameBuffers) {
        vkDestroyFramebuffer(rt.device, frameBuffer, nullptr);
    }

    vkDestroyPipeline(rt.device, rt.rayTracingPipeline, nullptr);
    vkDestroyPipelineLayout(rt.device, rt.pipeLineLayout, nullptr);

    vkDestroyPipelineLayout(rt.device, rt.pipeLineLayout, nullptr);
    vkDestroyRenderPass(rt.device, rt.renderPass, nullptr);

    for (auto imageView : rt.SWChainImageViews) {
        vkDestroyImageView(rt.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(rt.device, rt.swapChain, nullptr);
    vkDestroyDevice(rt.device, nullptr);

    if (rt.enableValLayers) {
        rt.DestroyDebugUtilsMessengerEXT(rt.instance, rt.debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(rt.instance, rt.surface, nullptr);
    vkDestroyInstance(rt.instance, nullptr);
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
    cleanupRT();
    SDL_DestroyWindow(displayWindow);
  
    SDL_Quit();
}

void executeRayTrace(Display d) {
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
        d.drawNewFrameRT(rt, MAX_FRAMES_IN_FLIGHT, rt.inFlightFences, rt.imagesInFlight);
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

    glm::mat4 translationMatrix{ 1.0f };

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

    rt.initializeRT();

    rt.createCommandPool();

    rt.createSWChain(displayWindow);
    rt.createImageViews();
    rt.createDepthResources();
    rt.createRenderPass();
    rt.createFrameBuffer();

    rt.loadModel(translationMatrix);

    rt.createStorageImage();

    rt.createVertexBuffer();
    rt.createIndexBuffer();
    rt.createMaterialBuffer();
    rt.createMaterialIndexBuffer();
    rt.createUniformBuffers();
    rt.createObjDescBuffers();

    rt.createBottomLevelAS();
    rt.createTopLevelAS();

    rt.createTextureImage();
    rt.createTextureImageView();
    rt.createTextureImageSampler();

    rt.createDescriptorPool();
    rt.createDescriptorSetLayout();
    rt.createRTDescriptorSet();

    rt.createRtPipeline();
    rt.createRayTraceShaderBindingTable();

    //rt.createCommandBuffers();

    rt.createSemaphores(MAX_FRAMES_IN_FLIGHT);

    glm::vec4 clearColor = glm::vec4(1, 1, 1, 1.00f);

    VkCommandBuffer cmdbuff = rt.beginSingleTimeCommands();

    rt.traceRay(cmdbuff, clearColor);

    rt.endSingleTimeCommands(cmdbuff);
}

int main(int argc, char** arcgv) {

    Display d;
    displayWindow = d.initDisplay("Vulkan Game Engine");

    //initVulkan();
    initRayTrace();

    //executeVulkanSDLLoop(d);
    executeRayTrace(d);

    return 0;
}