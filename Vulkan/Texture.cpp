
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Texture.h"
#include "VulkanHelpers.h"

#include <cstring>
#include <stdexcept>
#include <tuple>

void Texture::loadFromFile(VulkanContext& ctx, const std::string& path)
{
    // Charger les pixels
    int texWidth = 0, texHeight = 0, texChannels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image: " + path);
    }
    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * texHeight * 4;

    // Staging buffer
    auto [stagingBuffer, stagingMemory] = vkh::createBuffer(
        ctx, imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = stagingMemory.mapMemory(0, imageSize);
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    stagingMemory.unmapMemory();
    stbi_image_free(pixels);

    // Image GPU
    std::tie(image, memory) = vkh::createImage(
        ctx, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    //Upload 
    vk::raii::CommandBuffer cmd = vkh::beginSingleTimeCommands(ctx);
    vkh::transitionTextureLayout(cmd, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    vkh::copyBufferToImage(cmd, stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    vkh::transitionTextureLayout(cmd, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    vkh::endSingleTimeCommands(ctx, std::move(cmd));

    //  Vue
    view = vkh::createImageView(ctx, *image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

    // Sampler
    vk::PhysicalDeviceProperties properties = ctx.physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{
        .magFilter        = vk::Filter::eLinear,
        .minFilter        = vk::Filter::eLinear,
        .mipmapMode       = vk::SamplerMipmapMode::eLinear,
        .addressModeU     = vk::SamplerAddressMode::eRepeat,
        .addressModeV     = vk::SamplerAddressMode::eRepeat,
        .addressModeW     = vk::SamplerAddressMode::eRepeat,
        .mipLodBias       = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
        .compareEnable    = vk::False,
        .compareOp        = vk::CompareOp::eAlways};
    sampler = vk::raii::Sampler(ctx.device, samplerInfo);
}