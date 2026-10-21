#pragma once
#include "VulkanContext.h"
#include "Vertex.h"

class Mesh
{
public:
    void create(VulkanContext& ctx, std::vector<Vertex> vertices, std::vector<uint16_t> indices);
    void bindAndDraw(vk::raii::CommandBuffer& cmd) const
    {
        cmd.bindVertexBuffers(0, *vertexBuffer, {0});
        cmd.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
        cmd.drawIndexed(indexCount, 1, 0, 0, 0);
    }

private:
    vk::raii::Buffer       vertexBuffer = nullptr;
    vk::raii::DeviceMemory vertexMemory = nullptr;
    vk::raii::Buffer       indexBuffer  = nullptr;
    vk::raii::DeviceMemory indexMemory  = nullptr;
    uint32_t               indexCount   = 0;
};