// Ces defines doivent précéder tout include de glm dans ce fichier
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Descriptors.h"
#include "VulkanHelpers.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <chrono>
#include <cstring>


void Descriptors::createLayout(VulkanContext& ctx)
{
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{
        {.binding         = 0,
         .descriptorType  = vk::DescriptorType::eUniformBuffer,
         .descriptorCount = 1,
         .stageFlags      = vk::ShaderStageFlagBits::eVertex},
        {.binding         = 1,
         .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
         .descriptorCount = 1,
         .stageFlags      = vk::ShaderStageFlagBits::eFragment}}};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings    = bindings.data()};
    layout = vk::raii::DescriptorSetLayout(ctx.device, layoutInfo);
}


void Descriptors::create(VulkanContext& ctx, const Texture& tex)
{
    // uniform buffer 
    for (uint32_t i = 0; i < FRAMES; i++)
    {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        auto [buffer, memory] = vkh::createBuffer(
            ctx, bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        uniformBuffers.emplace_back(std::move(buffer));
        uniformMemory.emplace_back(std::move(memory));
        uniformMapped.emplace_back(uniformMemory.back().mapMemory(0, bufferSize));
    }

    // Pool
    std::array<vk::DescriptorPoolSize, 2> poolSizes{{
        {.type = vk::DescriptorType::eUniformBuffer,        .descriptorCount = FRAMES},
        {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = FRAMES}}};

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = FRAMES,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data()};
    pool = vk::raii::DescriptorPool(ctx.device, poolInfo);

    // Sets
    std::vector<vk::DescriptorSetLayout> layouts(FRAMES, *layout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool     = *pool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()};

    sets.clear();
    sets = ctx.device.allocateDescriptorSets(allocInfo);
    
    for (uint32_t i = 0; i < FRAMES; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = *uniformBuffers[i],
            .offset = 0,
            .range  = sizeof(UniformBufferObject)};

        vk::DescriptorImageInfo imageInfo{
            .sampler     = *tex.sampler,
            .imageView   = *tex.view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        std::array<vk::WriteDescriptorSet, 2> writes{{
            {.dstSet          = *sets[i],
             .dstBinding      = 0,
             .dstArrayElement = 0,
             .descriptorCount = 1,
             .descriptorType  = vk::DescriptorType::eUniformBuffer,
             .pBufferInfo     = &bufferInfo},
            {.dstSet          = *sets[i],
             .dstBinding      = 1,
             .dstArrayElement = 0,
             .descriptorCount = 1,
             .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
             .pImageInfo      = &imageInfo}}};

        ctx.device.updateDescriptorSets(writes, {});
    }
}

// Update UBO (ancien updateUniformBuffer)
void Descriptors::update(uint32_t frame, vk::Extent2D extent)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time        = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj  = glm::perspective(glm::radians(45.0f),
                                 static_cast<float>(extent.width) / static_cast<float>(extent.height),
                                 0.1f, 10.0f);

    std::memcpy(uniformMapped[frame], &ubo, sizeof(ubo));
}