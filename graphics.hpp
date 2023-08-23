#pragma once

#include <vulkan/vulkan.hpp>
#include <utility>
#include <glfwpp/glfwpp.h>

class Graphics {
public:
    explicit Graphics(glfw::Window &window);
private:
    vk::UniqueInstance   m_instance;
    vk::UniqueSurfaceKHR m_surface;
};
