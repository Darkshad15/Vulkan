#include "GraphicsPipeline.h"
#include "Vertex.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file: " + filename);
        }
        std::vector<char> buffer(static_cast<size_t>(file.tellg()));
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        return buffer;
    }

    vk::raii::ShaderModule createShaderModule(VulkanContext& ctx, const std::vector<char>& code)
    {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size(),
            .pCode    = reinterpret_cast<const uint32_t*>(code.data())};
        return vk::raii::ShaderModule(ctx.device, createInfo);
    }
}

void GraphicsPipeline::create(VulkanContext& ctx,
                              vk::Format colorFormat,
                              vk::Format depthFormat,
                              vk::DescriptorSetLayout descriptorLayout)
{
    // Chemin du shader
    vk::raii::ShaderModule shaderModule = createShaderModule(ctx, readFile("shader/slang.spv"));

    vk::PipelineShaderStageCreateInfo vertStage{
        .stage = vk::ShaderStageFlagBits::eVertex,   .module = shaderModule, .pName = "vertMain"};
    vk::PipelineShaderStageCreateInfo fragStage{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInput{
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data()};

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList};

    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eNone,
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f};

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable  = vk::False};

    // Test de profondeur 
    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable       = vk::True,
        .depthWriteEnable      = vk::True,
        .depthCompareOp        = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False};

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable    = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable   = vk::False,
        .logicOp         = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachment};

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data()};

    vk::PipelineLayoutCreateInfo layoutInfo{
        .setLayoutCount         = 1,
        .pSetLayouts            = &descriptorLayout,
        .pushConstantRangeCount = 0};
    layout = vk::raii::PipelineLayout(ctx.device, layoutInfo);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> chain = {
        {.stageCount          = 2,
         .pStages             = shaderStages,
         .pVertexInputState   = &vertexInput,
         .pInputAssemblyState = &inputAssembly,
         .pViewportState      = &viewportState,
         .pRasterizationState = &rasterizer,
         .pMultisampleState   = &multisampling,
         .pDepthStencilState  = &depthStencil,
         .pColorBlendState    = &colorBlending,
         .pDynamicState       = &dynamicState,
         .layout              = *layout,
         .renderPass          = nullptr},
        {.colorAttachmentCount    = 1,
         .pColorAttachmentFormats = &colorFormat,
         .depthAttachmentFormat   = depthFormat}};

    pipeline = vk::raii::Pipeline(ctx.device, nullptr, chain.get<vk::GraphicsPipelineCreateInfo>());
}