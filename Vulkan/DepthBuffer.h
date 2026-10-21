#pragma once
#include "VulkanContext.h"

class DepthBuffer
{
public:
    void create(VulkanContext& ctx, vk::Extent2D extent);   
    static vk::Format findFormat(VulkanContext& ctx);       

    vk::raii::Image        image  = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::raii::ImageView    view   = nullptr;
    vk::Format             format = vk::Format::eUndefined;
};