#include "Renderer.h"
#include "VulkanHelpers.h"

#include <stdexcept>
#include <cassert>

namespace
{
    // Données de la scène 
    const std::vector<Vertex> vertices = {
        // Face avant (z = 0)
        {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

        // Face arrière (z = -1)
        {{ 0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

        // Face gauche (x = -0.5)
        {{-0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

        // Face droite (x = 0.5)
        {{ 0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

        // Face dessus (y = 0.5)
        {{-0.5f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

        // Face dessous (y = -0.5)
        {{-0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}
    };


    const std::vector<uint16_t> indices = {
        0,  1,  2,  2,  3,  0,   // avant
        4,  5,  6,  6,  7,  4,   // arrière
        8,  9, 10, 10, 11,  8,   // gauche
       12, 13, 14, 14, 15, 12,   // droite
       16, 17, 18, 18, 19, 16,   // dessus
       20, 21, 22, 22, 23, 20    // dessous
   };

}


// Initialisation 
void Renderer::init(GLFWwindow* window)
{
    ctx.init(window);                                   
    swapchain.create(ctx, window);                      
    depth.create(ctx, swapchain.extent);                

    descriptors.createLayout(ctx);                      
    pipeline.create(ctx,
                    swapchain.surfaceFormat.format,
                    depth.format,
                    *descriptors.layout);

    texture.loadFromFile(ctx, "textures/texture.jpg");  
    mesh.create(ctx, vertices, indices);                
    descriptors.create(ctx, texture);                   

    createCommandBuffers();
    createSyncObjects();
}


// Command buffers et synchronisation
void Renderer::createCommandBuffers()
{
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool        = ctx.commandPool,
        .level              = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = vk::raii::CommandBuffers(ctx.device, allocInfo);
}

void Renderer::createRenderFinishedSemaphores()
{
    renderFinishedSemaphores.clear();
    for (size_t i = 0; i < swapchain.images.size(); i++)
    {
        renderFinishedSemaphores.emplace_back(ctx.device, vk::SemaphoreCreateInfo());
    }
}

void Renderer::createSyncObjects()
{
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

    createRenderFinishedSemaphores();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        presentCompleteSemaphores.emplace_back(ctx.device, vk::SemaphoreCreateInfo());
        inFlightFences.emplace_back(ctx.device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}


// Enregistrement des commandes d'une frame
void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
    auto& cmd = commandBuffers[frameIndex];
    cmd.begin({});

    // Swapchain : UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
    vkh::transitionImage(cmd, swapchain.images[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor);

    // Profondeur : UNDEFINED -> DEPTH_ATTACHMENT_OPTIMAL
    vkh::transitionImage(cmd, *depth.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth);

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo colorAttachment{
        .imageView   = swapchain.imageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor};

    vk::RenderingAttachmentInfo depthAttachment{
        .imageView   = depth.view,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eDontCare,
        .clearValue  = clearDepth};

    vk::RenderingInfo renderingInfo{
        .renderArea           = {.offset = {0, 0}, .extent = swapchain.extent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment,
        .pDepthAttachment     = &depthAttachment};

    cmd.beginRendering(renderingInfo);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);

    
    cmd.setViewport(0, vk::Viewport(0.0f,
                                    static_cast<float>(swapchain.extent.height),
                                    static_cast<float>(swapchain.extent.width),
                                    -static_cast<float>(swapchain.extent.height),
                                    0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain.extent));

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                           pipeline.layout, 0,
                           *descriptors.sets[frameIndex], nullptr);

    mesh.bindAndDraw(cmd);          
    cmd.endRendering();

    // Swapchain : COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
    vkh::transitionImage(cmd, swapchain.images[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::ImageAspectFlagBits::eColor);

    cmd.end();
}


// Une frame
void Renderer::drawFrame(GLFWwindow* window)
{

    auto fenceResult = ctx.device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to wait for fence!");
    }

    auto [result, imageIndex] = swapchain.swapChain.acquireNextImage(
        UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain(window);
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    descriptors.update(frameIndex, swapchain.extent);
    
    ctx.device.resetFences(*inFlightFences[frameIndex]);

    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]};
    ctx.queue.submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapchain.swapChain,
        .pImageIndices      = &imageIndex};
    result = ctx.queue.presentKHR(presentInfo);

    if (result == vk::Result::eSuboptimalKHR ||
        result == vk::Result::eErrorOutOfDateKHR ||
        framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain(window);
    }
    else
    {
        assert(result == vk::Result::eSuccess);
    }

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}


void Renderer::recreateSwapChain(GLFWwindow* window)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while ((width == 0 || height == 0) && !glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    if (glfwWindowShouldClose(window))
    {
        return;
    }

    ctx.device.waitIdle();

    swapchain.cleanup();
    swapchain.create(ctx, window);
    depth.create(ctx, swapchain.extent);    
    createRenderFinishedSemaphores();       
}