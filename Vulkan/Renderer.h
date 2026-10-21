#pragma once

#include "VulkanContext.h"
#include "Swapchain.h"
#include "DepthBuffer.h"
#include "Texture.h"
#include "Mesh.h"
#include "Descriptors.h"
#include "GraphicsPipeline.h"

#include <vector>

class Renderer
{
public:
    void init(GLFWwindow* window);
    void drawFrame(GLFWwindow* window);
    void waitIdle() { ctx.device.waitIdle(); }

    bool framebufferResized = false;

private:
    void createCommandBuffers();
    void createSyncObjects();
    void createRenderFinishedSemaphores();
    void recordCommandBuffer(uint32_t imageIndex);
    void recreateSwapChain(GLFWwindow* window);

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = Descriptors::FRAMES;


    VulkanContext    ctx;
    Swapchain        swapchain;
    DepthBuffer      depth;
    Texture          texture;
    Mesh             mesh;
    Descriptors      descriptors;
    GraphicsPipeline pipeline;

    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::Semaphore>     presentCompleteSemaphores;  // indexés par frameIndex
    std::vector<vk::raii::Semaphore>     renderFinishedSemaphores;   // indexés par imageIndex
    std::vector<vk::raii::Fence>         inFlightFences;             // indexés par frameIndex
    uint32_t                             frameIndex = 0;
};