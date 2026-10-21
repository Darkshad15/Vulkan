#pragma once

#include "VulkanContext.h"

#include <utility>

// Fonctions utilitaires / le contexte en paramètre.
namespace vkh
{
    // Buffers
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
    createBuffer(VulkanContext& ctx,
                 vk::DeviceSize size,
                 vk::BufferUsageFlags usage,
                 vk::MemoryPropertyFlags properties);

    void copyBuffer(VulkanContext& ctx,
                    vk::raii::Buffer& src,
                    vk::raii::Buffer& dst,
                    vk::DeviceSize size);

    // Images 
    std::pair<vk::raii::Image, vk::raii::DeviceMemory>
    createImage(VulkanContext& ctx,
                uint32_t width, uint32_t height,
                vk::Format format,
                vk::ImageTiling tiling,
                vk::ImageUsageFlags usage,
                vk::MemoryPropertyFlags properties);

    vk::raii::ImageView createImageView(VulkanContext& ctx,
                                        vk::Image image,
                                        vk::Format format,
                                        vk::ImageAspectFlags aspectFlags);

    void copyBufferToImage(vk::raii::CommandBuffer& cmd,
                           const vk::raii::Buffer& buffer,
                           vk::raii::Image& image,
                           uint32_t width, uint32_t height);

    //Command buffers ponctuels
    vk::raii::CommandBuffer beginSingleTimeCommands(VulkanContext& ctx);
    void endSingleTimeCommands(VulkanContext& ctx, vk::raii::CommandBuffer&& cmd);

    // Transitions de layout 
    void transitionTextureLayout(vk::raii::CommandBuffer& cmd,
                                 const vk::raii::Image& image,
                                 vk::ImageLayout oldLayout,
                                 vk::ImageLayout newLayout);
    
    // Transition de l'image
    void transitionImage(vk::raii::CommandBuffer& cmd,
                         vk::Image image,
                         vk::ImageLayout oldLayout,
                         vk::ImageLayout newLayout,
                         vk::AccessFlags2 srcAccessMask,
                         vk::AccessFlags2 dstAccessMask,
                         vk::PipelineStageFlags2 srcStageMask,
                         vk::PipelineStageFlags2 dstStageMask,
                         vk::ImageAspectFlags aspectFlags);
}