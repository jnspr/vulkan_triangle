#pragma once

#include <vector>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <glfwpp/glfwpp.h>
#include <shaderc/shaderc.hpp>

using SpirvCode = std::vector<uint32_t>;

class Graphics {
public:
    explicit Graphics(glfw::Window &window);
    ~Graphics();

    void renderFrame();
private:
    glfw::Window                      &m_window;
    vk::UniqueInstance                 m_instance;
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
    void createCommandBuffer();

    // Rendering
    void recordCommandBuffer(uint32_t imageIndex);
};
