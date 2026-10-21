#pragma once

#include "VulkanContext.h"
#include "Texture.h"
#include "Vertex.h"

#include <vector>

class Descriptors
{
public:
    static constexpr uint32_t FRAMES = 2;

    void createLayout(VulkanContext& ctx);               
    void create(VulkanContext& ctx, const Texture& tex);
    void update(uint32_t frame, vk::Extent2D extent);    

    vk::raii::DescriptorSetLayout layout = nullptr;

private:
    std::vector<vk::raii::Buffer>       uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformMemory;
    std::vector<void*>                  uniformMapped;
    
    vk::raii::DescriptorPool pool = nullptr;

public:
    std::vector<vk::raii::DescriptorSet> sets;
};