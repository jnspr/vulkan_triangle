#pragma once

#include <vector>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <glfwpp/glfwpp.h>

class Graphics {
public:
    explicit Graphics(glfw::Window &window);
    ~Graphics();
private:
    glfw::Window                      &m_window;
    vk::UniqueInstance                 m_instance;
    vk::UniqueSurfaceKHR               m_surface;
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

    // Device and presentation setup
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createRenderPass();
    void createImageViews();
    void createFramebuffers();
};
