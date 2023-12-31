#pragma once

#include "graphics.hpp"
#include "pch.hpp"

#include <utility>

class Application {
public:
    Application();

    void runUntilClose();
private:
    glfw::GlfwLibrary m_glfw;
    glfw::Window      m_window;
    Graphics          m_graphics;
    bool              m_mustResize;

    static glfw::Window createVulkanWindow(int width, int height, const char *title);
};
