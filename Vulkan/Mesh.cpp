#include "Mesh.h"
#include "VulkanHelpers.h"

#include <cstring>
#include <tuple>

namespace
{
    
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
    uploadToDeviceLocal(VulkanContext& ctx, const void* data, vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        auto [stagingBuffer, stagingMemory] = vkh::createBuffer(
            ctx, size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mapped = stagingMemory.mapMemory(0, size);
        std::memcpy(mapped, data, static_cast<size_t>(size));
        stagingMemory.unmapMemory();

        auto result = vkh::createBuffer(
            ctx, size,
            usage | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        vkh::copyBuffer(ctx, stagingBuffer, result.first, size);
        return result;
    }
}

void Mesh::create(VulkanContext& ctx, std::vector<Vertex> vertices, std::vector<uint16_t> indices)
{
    indexCount = static_cast<uint32_t>(indices.size());

    std::tie(vertexBuffer, vertexMemory) = uploadToDeviceLocal(
        ctx, vertices.data(), sizeof(Vertex) * vertices.size(),
        vk::BufferUsageFlagBits::eVertexBuffer);

    std::tie(indexBuffer, indexMemory) = uploadToDeviceLocal(
        ctx, indices.data(), sizeof(uint16_t) * indices.size(),
        vk::BufferUsageFlagBits::eIndexBuffer);
}