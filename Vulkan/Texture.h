#pragma once
#include "VulkanContext.h"
#include <string>

class Texture
{
public:
    void loadFromFile(VulkanContext& ctx, const std::string& path);  // image + view + sampler

    vk::raii::Image        image   = nullptr;
    vk::raii::DeviceMemory memory  = nullptr;
    vk::raii::ImageView    view    = nullptr;
    vk::raii::Sampler      sampler = nullptr;
};