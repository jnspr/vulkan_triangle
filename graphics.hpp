#pragma once

#include <vulkan/vulkan.hpp>
#include <utility>
#include <glfwpp/glfwpp.h>

class Graphics {
public:
    explicit Graphics(glfw::Window &window);
private:
    vk::UniqueInstance     m_instance;
    vk::UniqueSurfaceKHR   m_surface;
    vk::PhysicalDevice     m_physicalDevice;
    vk::SurfaceFormatKHR   m_surfaceFormat;
    uint32_t               m_queueFamilyIndex;
    vk::UniqueDevice       m_logicalDevice;
    vk::Queue              m_queue;
    vk::UniqueSwapchainKHR m_swapchain;

    // Device and presentation setup
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(vk::Extent2D imageExtent);
};
