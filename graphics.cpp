#include "graphics.hpp"

Graphics::Graphics(glfw::Window &window) {
    // Create instance with extensions for presenting to window surface
    auto extensions = glfw::getRequiredInstanceExtensions();
    m_instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo()
            .setPEnabledExtensionNames(extensions)
    );

    // Create surface for presenting
    m_surface = vk::UniqueSurfaceKHR(window.createSurface(*m_instance), *m_instance);
}
