#include "graphics.hpp"

void Graphics::createShaders() {
    // Create modules from the SPIR-V shaders
    m_shaderModules[0] = m_logicalDevice->createShaderModuleUnique(
        vk::ShaderModuleCreateInfo()
            .setCode(m_vertexShaderCode)
    );
    m_shaderModules[1] = m_logicalDevice->createShaderModuleUnique(
        vk::ShaderModuleCreateInfo()
            .setCode(m_fragmentShaderCode)
    );

    // Populate the shader stage creation infos for later use
    m_shaderStages[0] = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(*m_shaderModules[0])
        .setPName("main");
    m_shaderStages[1] = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(*m_shaderModules[1])
        .setPName("main");
}

void Graphics::initViewportAndScissor() {
    m_viewport = vk::Viewport()
        .setX(0.0)
        .setY(0.0)
        .setWidth(static_cast<float>(m_imageExtent.width))
        .setHeight(static_cast<float>(m_imageExtent.height))
        .setMinDepth(0.0)
        .setMaxDepth(1.0);
    m_scissor = vk::Rect2D()
        .setOffset({0, 0})
        .setExtent(m_imageExtent);
}

void Graphics::createGraphicsPipeline() {
    // Define the viewport and scissor to be dynamic
    static const vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor,
    };
    auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo()
        .setPDynamicStates(dynamicStates)
        .setDynamicStateCount(2);

    // Define a vertex input state based on the vertex structure
    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
        .setPVertexBindingDescriptions(&vertexBindingDescription)
        .setVertexBindingDescriptionCount(1)
        .setVertexAttributeDescriptions(vertexAttributeDescriptions);

    // Define input vertices to be organized as a list of triangles
    auto inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(VK_FALSE);

    // Define a rasterizer that fills polygons and culls back-faces
    auto rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(VK_FALSE)
        .setRasterizerDiscardEnable(VK_FALSE)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setDepthBiasEnable(VK_FALSE);

    // Define multisampling to be disabled
    auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo()
        .setSampleShadingEnable(VK_FALSE)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setMinSampleShading(1.0)
        .setAlphaToCoverageEnable(VK_FALSE)
        .setAlphaToOneEnable(VK_FALSE);

    // Define color blending to be disabled
    auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(VK_FALSE)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);
    auto colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
        .setLogicOpEnable(VK_FALSE)
        .setLogicOp(vk::LogicOp::eCopy)
        .setPAttachments(&colorBlendAttachment)
        .setAttachmentCount(1)
        .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});

    // Define the viewport's initial state
    auto viewportInfo = vk::PipelineViewportStateCreateInfo()
        .setPViewports(&m_viewport)
        .setViewportCount(1)
        .setPScissors(&m_scissor)
        .setScissorCount(1);

    // Create an empty pipeline layout
    m_graphicsPipelineLayout = m_logicalDevice->createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo()
    );

    m_graphicsPipeline = m_logicalDevice->createGraphicsPipelineUnique(
        nullptr,
        vk::GraphicsPipelineCreateInfo()
            .setRenderPass(*m_renderPass)
            .setPStages(m_shaderStages)
            .setStageCount(2)
            .setPDynamicState(&dynamicStateInfo)
            .setPVertexInputState(&vertexInputInfo)
            .setPInputAssemblyState(&inputAssemblyInfo)
            .setPRasterizationState(&rasterizationInfo)
            .setPMultisampleState(&multisampleInfo)
            .setPColorBlendState(&colorBlendInfo)
            .setPViewportState(&viewportInfo)
            .setLayout(*m_graphicsPipelineLayout)
            .setSubpass(0)
    ).value;
}

void Graphics::createCommandBuffer() {
    // Create a command pool
    m_commandPool = m_logicalDevice->createCommandPoolUnique(
        vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(m_queueFamilyIndex)
    );

    // Allocate a single command buffer
    auto commandBuffers = m_logicalDevice->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
            .setCommandPool(*m_commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1)
    );

    // Check if the allocation succeeded and move the buffer into `m_commandBuffer`
    if (commandBuffers.empty())
        throw std::runtime_error("Unable to allocate command buffer");
    m_commandBuffer = std::move(commandBuffers[0]);
}
