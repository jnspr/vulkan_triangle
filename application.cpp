#include "application.hpp"

Application::Application():
    m_glfw(glfw::init()),
    m_window(createVulkanWindow(1280, 720, "vulkan_triangle")),
    m_graphics(m_window)
{
}

void Application::runUntilClose() {
    while (!m_window.shouldClose()) {
        glfw::pollEvents();
    }
}

glfw::Window Application::createVulkanWindow(int width, int height, const char *title) {
    // To support Vulkan, OpenGL must be disabled before window creation
    glfw::WindowHints hints = {};
    hints.clientApi = glfw::ClientApi::None;
    hints.apply();
    return glfw::Window(width, height, title);
}
