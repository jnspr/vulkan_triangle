#pragma once

#include <utility>
#include <glfwpp/glfwpp.h>

class Application {
public:
    Application();

    void runUntilClose();
private:
    glfw::GlfwLibrary m_glfw;
    glfw::Window      m_window;
};
