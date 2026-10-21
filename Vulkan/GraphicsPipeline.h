#pragma once
#include "VulkanContext.h"

class GraphicsPipeline
{
public:
    void create(VulkanContext& ctx, vk::Format colorFormat, vk::Format depthFormat,
                vk::DescriptorSetLayout descriptorLayout);

    vk::raii::PipelineLayout layout   = nullptr;
    vk::raii::Pipeline       pipeline = nullptr;
};