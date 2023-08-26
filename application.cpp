#include "application.hpp"

Application::Application():
    m_glfw(glfw::init()),
    m_window(createVulkanWindow(1280, 720, "vulkan_triangle")),
    m_graphics(m_window)
{
    m_window.framebufferSizeEvent.setCallback([this](glfw::Window &_window, int _width, int _height) {
        m_mustResize = true;
    });
}

void Application::runUntilClose() {
    while (!m_window.shouldClose()) {
        glfw::pollEvents();
        if (m_mustResize) {
            m_graphics.handleResize();
            m_mustResize = false;
        }
        m_graphics.renderFrame();
    }
}

glfw::Window Application::createVulkanWindow(int width, int height, const char *title) {
    // To support Vulkan, OpenGL must be disabled before window creation
    glfw::WindowHints hints = {};
    hints.clientApi = glfw::ClientApi::None;
    hints.apply();
    return glfw::Window(width, height, title);
}
