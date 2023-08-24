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
    vk::Queue                          m_queue;
    vk::Extent2D                       m_imageExtent;
    vk::UniqueSwapchainKHR             m_swapchain;
    vk::UniqueRenderPass               m_renderPass;
    std::vector<vk::UniqueImageView>   m_imageViews;
    std::vector<vk::UniqueFramebuffer> m_framebuffers;

    // Preparation
    void createInstanceAndSurface();
    static SpirvCode loadAndCompileShader(shaderc::Compiler &compiler, shaderc_shader_kind kind, const char *path);
    void loadAndCompileShaders();

    // Device and presentation setup
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createRenderPass();
    void createImageViews();
    void createFramebuffers();
};
