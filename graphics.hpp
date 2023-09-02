#pragma once

#include "pch.hpp"

#include <array>
#include <vector>

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    static inline auto getBindingDescription() {
        return vk::VertexInputBindingDescription()
            .setBinding(0)
            .setStride(sizeof(Vertex))
            .setInputRate(vk::VertexInputRate::eVertex);
    }

    static inline auto getAttributeDescriptions() {
        return std::array<vk::VertexInputAttributeDescription, 2> {
            vk::VertexInputAttributeDescription()
                .setBinding(0)
                .setLocation(0)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setOffset(offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription()
                .setBinding(0)
                .setLocation(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(offsetof(Vertex, color))
        };
    }
};

using SpirvCode = std::vector<uint32_t>;

class Graphics {
public:
    explicit Graphics(glfw::Window &window);
    ~Graphics();

    void renderFrame();
    void handleResize();
private:
    static std::vector<Vertex>         k_vertexData;
    glfw::Window                      &m_window;
    vk::UniqueInstance                 m_instance;
#ifdef ENABLE_VALIDATION
    vk::DispatchLoaderDynamic                                               m_dispatch;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_messenger;
#endif
    vk::UniqueSurfaceKHR               m_surface;
    SpirvCode                          m_vertexShaderCode;
    SpirvCode                          m_fragmentShaderCode;
    vk::PhysicalDevice                 m_physicalDevice;
    vk::SurfaceFormatKHR               m_surfaceFormat;
    uint32_t                           m_queueFamilyIndex;
    vk::UniqueDevice                   m_logicalDevice;
    vk::UniqueFence                    m_nextFrameFence;
    vk::UniqueSemaphore                m_imageAcquireSema;
    vk::UniqueSemaphore                m_renderFinishSema;
    vk::Queue                          m_queue;
    vk::Extent2D                       m_imageExtent;
    vk::UniqueSwapchainKHR             m_swapchain;
    vk::UniqueRenderPass               m_renderPass;
    std::vector<vk::UniqueImageView>   m_imageViews;
    std::vector<vk::UniqueFramebuffer> m_framebuffers;
    vk::UniqueShaderModule             m_shaderModules[2];
    vk::PipelineShaderStageCreateInfo  m_shaderStages[2];
    vk::Viewport                       m_viewport;
    vk::Rect2D                         m_scissor;
    vk::UniquePipelineLayout           m_graphicsPipelineLayout;
    vk::UniquePipeline                 m_graphicsPipeline;
    vk::UniqueBuffer                   m_vertexBuffer;
    vk::UniqueDeviceMemory             m_vertexMemory;
    vk::UniqueCommandPool              m_commandPool;
    vk::UniqueCommandBuffer            m_commandBuffer;

    // Preparation
    void createInstanceAndSurface();
    static SpirvCode loadAndCompileShader(shaderc::Compiler &compiler, shaderc_shader_kind kind, const char *path);
    void loadAndCompileShaders();

    // Device and presentation setup
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createRenderSync();
    void createSwapchain();
    void createRenderPass();
    void createImageViews();
    void createFramebuffers();

    // Rendering setup
    void createShaders();
    void initViewportAndScissor();
    void createGraphicsPipeline();
    void createVertexBuffer();
    void createCommandBuffer();

    // Object usage
    void recordCommandBuffer(uint32_t imageIndex);

    // Callback for debug messages
#ifdef ENABLE_VALIDATION
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void                                       *pUserData
    );
#endif
};
