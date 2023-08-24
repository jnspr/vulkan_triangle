#include "graphics.hpp"

#include <fstream>
#include <cstdint>
#include <stdexcept>

#ifdef ENABLE_VALIDATION
#include <iostream>
#endif

Graphics::Graphics(glfw::Window &window): m_window(window), m_queueFamilyIndex(0xffffffff) {
    // Preparation
    createInstanceAndSurface();
    loadAndCompileShaders();

    // Setup devices and presentation
    selectPhysicalDevice();
    createLogicalDevice();
    createRenderSync();
    createSwapchain();
    createRenderPass();
    createImageViews();
    createFramebuffers();

    // Rendering setup
    createShaders();
    initViewportAndScissor();
    createGraphicsPipeline();
    createCommandBuffer();
}

Graphics::~Graphics() {
    if (m_logicalDevice)
        m_logicalDevice->waitIdle();
}

void Graphics::renderFrame() {
    // Wait for the next frame
    vk::resultCheck(
        m_logicalDevice->waitForFences(1, &m_nextFrameFence.get(), VK_TRUE, UINT64_MAX),
        "vk::Device::waitForFences"
    );
    vk::resultCheck(
        m_logicalDevice->resetFences(1, &m_nextFrameFence.get()),
        "vk::Device::resetFences"
    );

    // Acquire the next image for rendering
    uint32_t imageIndex;
    vk::resultCheck(
        m_logicalDevice->acquireNextImageKHR(*m_swapchain, UINT64_MAX, *m_imageAcquireSema, {}, &imageIndex),
        "vk::Device::acquireNextImageKHR",
        { vk::Result::eSuccess, vk::Result::eSuboptimalKHR }
    );

    // Record and submit the command buffer
    recordCommandBuffer(imageIndex);
    auto waitStage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    auto submitInfo = vk::SubmitInfo()
        .setPWaitSemaphores(&m_imageAcquireSema.get())
        .setPWaitDstStageMask(&waitStage)
        .setWaitSemaphoreCount(1)
        .setPCommandBuffers(&m_commandBuffer.get())
        .setCommandBufferCount(1)
        .setPSignalSemaphores(&m_renderFinishSema.get())
        .setSignalSemaphoreCount(1);
    vk::resultCheck(m_queue.submit(1, &submitInfo, *m_nextFrameFence), "vk::Queue::submit");

    // Queue presentation to occur when rendering is finished
    vk::resultCheck(
        m_queue.presentKHR(vk::PresentInfoKHR()
            .setPWaitSemaphores(&m_renderFinishSema.get())
            .setWaitSemaphoreCount(1)
            .setPSwapchains(&m_swapchain.get())
            .setSwapchainCount(1)
            .setPImageIndices(&imageIndex)
        ),
        "vk::Queue::presentKHR",
        { vk::Result::eSuccess, vk::Result::eSuboptimalKHR }
    );
}

void Graphics::createInstanceAndSurface() {
    auto layers = std::vector<const char *>();

    // Get required extensions for presenting to a window
    auto extensions = glfw::getRequiredInstanceExtensions();

#ifdef ENABLE_VALIDATION
    // Optionally enable validation layer and debug callback extension
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back("VK_EXT_debug_utils");
#endif

    // Create instance with the collected extensions and layers
    m_instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo()
            .setPEnabledLayerNames(layers)
            .setPEnabledExtensionNames(extensions)
    );

#ifdef ENABLE_VALIDATION
    // Optionally create a dynamic dispatch for the debug extension and a messenger to the callback function
    m_dispatch = vk::DispatchLoaderDynamic(*m_instance, glfw::getInstanceProcAddress);
    m_messenger = m_instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
            .setPfnUserCallback(Graphics::debugCallback),
        nullptr,
        m_dispatch
    );
#endif

    // Create surface for presenting
    m_surface = vk::UniqueSurfaceKHR(m_window.createSurface(*m_instance), *m_instance);
}

SpirvCode Graphics::loadAndCompileShader(shaderc::Compiler &compiler, shaderc_shader_kind kind, const char *path) {
    std::vector<char> glslSource;
    {
        // Open the file as a stream and seek to its end
        std::ifstream stream(path, std::ios::ate | std::ios::binary);
        if (!stream.is_open())
            throw std::runtime_error("Unable to open shader source");

        // Use the end position as the buffer length and seek back to the start
        auto length = stream.tellg();
        stream.seekg(0);
        glslSource.resize(static_cast<size_t>(length));

        // Read the file into the buffer
        stream.read(glslSource.data(), length);
        if (!stream.good())
            throw std::runtime_error("Unable to read shader source");
    }

    // Compile the GLSL shader to SPIR-V and optimize for performance
    auto options = shaderc::CompileOptions();
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    auto result = compiler.CompileGlslToSpv(glslSource.data(), glslSource.size(), kind, "glsl_shader", "main", options);

    // Check for errors and return the SPIR-V code if successful
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        throw std::runtime_error("Unable to compile shader: " + result.GetErrorMessage());
    return SpirvCode(result.cbegin(), result.cend());
}

void Graphics::loadAndCompileShaders() {
    auto compiler = shaderc::Compiler();

    m_vertexShaderCode = loadAndCompileShader(compiler, shaderc_vertex_shader, "triangle.vert");
    m_fragmentShaderCode = loadAndCompileShader(compiler, shaderc_fragment_shader, "triangle.frag");
}

void Graphics::selectPhysicalDevice() {
    for (auto device : m_instance->enumeratePhysicalDevices()) {
        // Only use non-CPU devices
        auto properties = device.getProperties();
        if (properties.deviceType == vk::PhysicalDeviceType::eCpu)
            continue;

        // Only use devices where `m_surface` has at least one format
        auto formats = device.getSurfaceFormatsKHR(*m_surface);
        if (formats.empty())
            continue;

        auto queueFamilies = device.getQueueFamilyProperties();
        for (uint32_t index = 0; index < queueFamilies.size(); index++) {
            // Only use queue families which support graphics operations
            if (!(queueFamilies[index].queueFlags & vk::QueueFlagBits::eGraphics))
                continue;

            // Only use queue families which support graphics and present operations on `m_surface`
            if (!device.getSurfaceSupportKHR(index, *m_surface))
                continue;
            if (!glfw::getPhysicalDevicePresentationSupport(*m_instance, device, index))
                continue;

            m_physicalDevice = device;
            m_surfaceFormat = formats[0];
            m_queueFamilyIndex = index;
            return;
        }
    }
    throw std::runtime_error("No supported physical device was found");
}

void Graphics::createLogicalDevice() {
    // Define a single queue (the graphics queue) that has to be created with the device
    const float queuePriority = 1.0f;
    auto queueCreateInfo = vk::DeviceQueueCreateInfo()
        .setQueueFamilyIndex(m_queueFamilyIndex)
        .setPQueuePriorities(&queuePriority)
        .setQueueCount(1);

    // Create a logical device with swapchain support
    static const char *swapchainExtension = "VK_KHR_swapchain";
    m_logicalDevice = m_physicalDevice.createDeviceUnique(
        vk::DeviceCreateInfo()
            .setPpEnabledExtensionNames(&swapchainExtension)
            .setEnabledExtensionCount(1)
            .setPQueueCreateInfos(&queueCreateInfo)
            .setQueueCreateInfoCount(1)
    );

    // Obtain the created queue's handle
    m_queue = m_logicalDevice->getQueue(m_queueFamilyIndex, 0);
}

void Graphics::createRenderSync() {
    // Create a fence which prevents the render loop from acquiring the next image until the current image is rendered,
    // it is initialized to be in signalled state so the first render can occur
    m_nextFrameFence = m_logicalDevice->createFenceUnique(
        vk::FenceCreateInfo()
            .setFlags(vk::FenceCreateFlagBits::eSignaled)
    );

    // Create a semaphore which signals the graphics pipeline that an image is ready to be drawn on
    m_imageAcquireSema = m_logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());

    // Create a semaphore which signals the swapchain that rendering has finished and the image can be presented
    m_renderFinishSema = m_logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
}

void Graphics::createSwapchain() {
    auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
    auto extentTuple = m_window.getFramebufferSize();
    m_imageExtent = vk::Extent2D(std::get<0>(extentTuple), std::get<1>(extentTuple));

    // Check if the image extent is within the allowed range
    if (m_imageExtent.width  < capabilities.minImageExtent.width  ||
        m_imageExtent.height < capabilities.minImageExtent.height ||
        m_imageExtent.width  > capabilities.maxImageExtent.width  ||
        m_imageExtent.height > capabilities.maxImageExtent.height)
    {
        throw std::runtime_error("Unable create swapchain with given image extent");
    }

    uint32_t minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount != 0 && minImageCount > capabilities.maxImageCount)
        minImageCount = capabilities.maxImageCount;

    m_swapchain = m_logicalDevice->createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
            // Present to `m_surface` using a FIFO
            .setSurface(*m_surface)
            .setPresentMode(vk::PresentModeKHR::eFifo)
            // Use the previously selected graphics queue family
            .setPQueueFamilyIndices(&m_queueFamilyIndex)
            .setQueueFamilyIndexCount(1)
            // Swap through `minImageCount` images with color attachment
            .setImageFormat(m_surfaceFormat.format)
            .setImageColorSpace(m_surfaceFormat.colorSpace)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageExtent(m_imageExtent)
            .setImageArrayLayers(1)
            .setMinImageCount(minImageCount)
    );
}

void Graphics::createRenderPass() {
    // Define the color attachment
    auto colorDescription = vk::AttachmentDescription()
        .setFormat(m_surfaceFormat.format)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    // Define a single subpass which reference the color attachment
    auto colorReference = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    auto subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setPColorAttachments(&colorReference)
        .setColorAttachmentCount(1);

    // Define a dependency for image acquisition before the render pass is started
    auto dependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eNone);

    m_renderPass = m_logicalDevice->createRenderPassUnique(
        vk::RenderPassCreateInfo()
            .setPAttachments(&colorDescription)
            .setAttachmentCount(1)
            .setPSubpasses(&subpass)
            .setSubpassCount(1)
            .setPDependencies(&dependency)
            .setDependencyCount(1)
    );
}

void Graphics::createImageViews() {
    // Define a view of an image's color attachment
    auto createInfo = vk::ImageViewCreateInfo()
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(m_surfaceFormat.format)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseMipLevel(0)
                .setLevelCount(1)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
        );

    // Reserve space for each new image view handle
    auto images = m_logicalDevice->getSwapchainImagesKHR(*m_swapchain);
    m_imageViews.reserve(images.size());

    // Create a view for each image in the swapchain
    for (auto image : images) {
        m_imageViews.push_back(
            m_logicalDevice->createImageViewUnique(createInfo.setImage(image))
        );
    }
}

void Graphics::createFramebuffers() {
    // Define a framebuffer of the previously set image extent with a color attachment
    auto createInfo = vk::FramebufferCreateInfo()
        .setRenderPass(*m_renderPass)
        .setAttachmentCount(1)
        .setWidth(m_imageExtent.width)
        .setHeight(m_imageExtent.height)
        .setLayers(1);

    // Reserve space for each new framebuffer
    m_framebuffers.reserve(m_imageViews.size());

    // Create a framebuffer for each image view
    for (auto &imageView : m_imageViews) {
        m_framebuffers.push_back(
            m_logicalDevice->createFramebufferUnique(createInfo.setPAttachments(&imageView.get()))
        );
    }
}

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

    // Define an empty vertex input state
    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo();

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

void Graphics::recordCommandBuffer(uint32_t imageIndex) {
    // Reset the buffer and start recording
    m_commandBuffer->reset();
    m_commandBuffer->begin(vk::CommandBufferBeginInfo());

    // Start the render pass with a solid black clear color
    auto clearValue = vk::ClearValue()
        .setColor({0.0f, 0.0f, 0.0f, 1.0f});
    m_commandBuffer->beginRenderPass(
        vk::RenderPassBeginInfo()
            .setRenderPass(*m_renderPass)
            .setFramebuffer(*m_framebuffers[imageIndex])
            .setRenderArea(m_scissor)
            .setPClearValues(&clearValue)
            .setClearValueCount(1),
        vk::SubpassContents::eInline
    );

    // Bind the graphics pipeline
    m_commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);

    // Set viewport and scissor
    m_commandBuffer->setViewport(0, 1, &m_viewport);
    m_commandBuffer->setScissor(0, 1, &m_scissor);

    // Draw the triangle
    m_commandBuffer->draw(3, 1, 0, 0);

    // End the render pass and recording
    m_commandBuffer->endRenderPass();
    m_commandBuffer->end();
}

#ifdef ENABLE_VALIDATION
VKAPI_ATTR VkBool32 VKAPI_CALL Graphics::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData
) {
    std::cerr << "[VK_EXT_debug_utils] " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
#endif
