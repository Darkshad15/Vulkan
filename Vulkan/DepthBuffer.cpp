#include "DepthBuffer.h"
#include "VulkanHelpers.h"

#include <stdexcept>
#include <tuple>
#include <vector>

vk::Format DepthBuffer::findFormat(VulkanContext& ctx)
{
    const std::vector<vk::Format> candidates = {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint};

    for (vk::Format candidate : candidates)
    {
        vk::FormatProperties props = ctx.physicalDevice.getFormatProperties(candidate);
        if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
            vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
            return candidate;
        }
    }
    throw std::runtime_error("failed to find supported depth format!");
}

void DepthBuffer::create(VulkanContext& ctx, vk::Extent2D extent)
{
    
    view   = nullptr;
    image  = nullptr;
    memory = nullptr;

    format = findFormat(ctx);

    std::tie(image, memory) = vkh::createImage(ctx,
                                               extent.width, extent.height,
                                               format,
                                               vk::ImageTiling::eOptimal,
                                               vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                               vk::MemoryPropertyFlagBits::eDeviceLocal);

    view = vkh::createImageView(ctx, *image, format, vk::ImageAspectFlagBits::eDepth);
}