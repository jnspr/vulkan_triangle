#pragma once

#include "graphics.hpp"

#include <utility>
#include <glfwpp/glfwpp.h>

class Application {
public:
    Application();

    void runUntilClose();
private:
    glfw::GlfwLibrary m_glfw;
    glfw::Window      m_window;
    Graphics          m_graphics;
};
