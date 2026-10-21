#include "VulkanHelpers.h"

#include <stdexcept>

namespace vkh
{

    // Buffers
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
    createBuffer(VulkanContext& ctx,
                 vk::DeviceSize size,
                 vk::BufferUsageFlags usage,
                 vk::MemoryPropertyFlags properties)
    {
        vk::BufferCreateInfo bufferInfo{
            .size        = size,
            .usage       = usage,
            .sharingMode = vk::SharingMode::eExclusive};
        vk::raii::Buffer buffer(ctx.device, bufferInfo);

        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = ctx.findMemoryType(memRequirements.memoryTypeBits, properties)};
        vk::raii::DeviceMemory memory(ctx.device, allocInfo);

        buffer.bindMemory(*memory, 0);
        return {std::move(buffer), std::move(memory)};
    }

    void copyBuffer(VulkanContext& ctx,
                    vk::raii::Buffer& src,
                    vk::raii::Buffer& dst,
                    vk::DeviceSize size)
    {
        vk::raii::CommandBuffer cmd = beginSingleTimeCommands(ctx);
        cmd.copyBuffer(*src, *dst, vk::BufferCopy{.size = size});
        endSingleTimeCommands(ctx, std::move(cmd));
    }

    // Images
    std::pair<vk::raii::Image, vk::raii::DeviceMemory>
    createImage(VulkanContext& ctx,
                uint32_t width, uint32_t height,
                vk::Format format,
                vk::ImageTiling tiling,
                vk::ImageUsageFlags usage,
                vk::MemoryPropertyFlags properties)
    {
        vk::ImageCreateInfo imageInfo{
            .imageType   = vk::ImageType::e2D,
            .format      = format,
            .extent      = {width, height, 1},
            .mipLevels   = 1,
            .arrayLayers = 1,
            .samples     = vk::SampleCountFlagBits::e1,
            .tiling      = tiling,
            .usage       = usage,
            .sharingMode = vk::SharingMode::eExclusive};
        vk::raii::Image image(ctx.device, imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = ctx.findMemoryType(memRequirements.memoryTypeBits, properties)};
        vk::raii::DeviceMemory memory(ctx.device, allocInfo);

        image.bindMemory(*memory, 0);
        return {std::move(image), std::move(memory)};
    }

    vk::raii::ImageView createImageView(VulkanContext& ctx,
                                        vk::Image image,
                                        vk::Format format,
                                        vk::ImageAspectFlags aspectFlags)
    {
        vk::ImageViewCreateInfo viewInfo{
            .image            = image,
            .viewType         = vk::ImageViewType::e2D,
            .format           = format,
            .subresourceRange = {.aspectMask     = aspectFlags,
                                 .baseMipLevel   = 0,
                                 .levelCount     = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount     = 1}};
        return vk::raii::ImageView(ctx.device, viewInfo);
    }

    void copyBufferToImage(vk::raii::CommandBuffer& cmd,
                           const vk::raii::Buffer& buffer,
                           vk::raii::Image& image,
                           uint32_t width, uint32_t height)
    {
        vk::BufferImageCopy region{
            .bufferOffset      = 0,
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource  = {.aspectMask     = vk::ImageAspectFlagBits::eColor,
                                  .mipLevel       = 0,
                                  .baseArrayLayer = 0,
                                  .layerCount     = 1},
            .imageOffset       = {0, 0, 0},
            .imageExtent       = {width, height, 1}};
        cmd.copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);
    }


    // Command buffers ponctuels
    vk::raii::CommandBuffer beginSingleTimeCommands(VulkanContext& ctx)
    {
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool        = ctx.commandPool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1};
        vk::raii::CommandBuffer cmd = std::move(vk::raii::CommandBuffers(ctx.device, allocInfo).front());

        cmd.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        return cmd;
    }

    void endSingleTimeCommands(VulkanContext& ctx, vk::raii::CommandBuffer&& cmd)
    {
        cmd.end();

        vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*cmd};
        ctx.queue.submit(submitInfo, nullptr);
        ctx.queue.waitIdle();
    }


    // Transitions de layout
    void transitionTextureLayout(vk::raii::CommandBuffer& cmd,
                                 const vk::raii::Image& image,
                                 vk::ImageLayout oldLayout,
                                 vk::ImageLayout newLayout)
    {
        vk::ImageMemoryBarrier barrier{
            .oldLayout           = oldLayout,
            .newLayout           = newLayout,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image               = *image,
            .subresourceRange    = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                                    .levelCount = 1,
                                    .layerCount = 1}};

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            sourceStage           = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage      = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
                 newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            sourceStage           = vk::PipelineStageFlagBits::eTransfer;
            destinationStage      = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        cmd.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
    }

    void transitionImage(vk::raii::CommandBuffer& cmd,
                         vk::Image image,
                         vk::ImageLayout oldLayout,
                         vk::ImageLayout newLayout,
                         vk::AccessFlags2 srcAccessMask,
                         vk::AccessFlags2 dstAccessMask,
                         vk::PipelineStageFlags2 srcStageMask,
                         vk::PipelineStageFlags2 dstStageMask,
                         vk::ImageAspectFlags aspectFlags)
    {
        vk::ImageMemoryBarrier2 barrier{
            .srcStageMask        = srcStageMask,
            .srcAccessMask       = srcAccessMask,
            .dstStageMask        = dstStageMask,
            .dstAccessMask       = dstAccessMask,
            .oldLayout           = oldLayout,
            .newLayout           = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange    = {.aspectMask     = aspectFlags,
                                    .baseMipLevel   = 0,
                                    .levelCount     = 1,
                                    .baseArrayLayer = 0,
                                    .layerCount     = 1}};

        vk::DependencyInfo dependencyInfo{
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier};

        cmd.pipelineBarrier2(dependencyInfo);
    }
}