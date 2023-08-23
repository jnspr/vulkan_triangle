#include "application.hpp"

Application::Application():
    m_glfw(glfw::init()),
    m_window(1280, 720, "vulkan_triangle")
{
}

void Application::runUntilClose() {
    while (!m_window.shouldClose()) {
        glfw::pollEvents();
    }
}
