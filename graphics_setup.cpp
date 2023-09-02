#include "graphics.hpp"

#include <fstream>
#include <cstdint>
#include <stdexcept>

#ifdef ENABLE_VALIDATION
#include <iostream>
#endif

Graphics::Graphics(glfw::Window &window): m_window(window), m_queueFamilyIndex(0xffffffff) {
    // Preparation
    createInstanceAndSurface();
    loadAndCompileShaders();

    // Setup devices and presentation
    selectPhysicalDevice();
    createLogicalDevice();
    createRenderSync();
    createSwapchain();
    createRenderPass();
    createImageViews();
    createFramebuffers();

    // Rendering setup
    createShaders();
    initViewportAndScissor();
    createGraphicsPipeline();
    createVertexBuffer();
    createCommandBuffer();
}

Graphics::~Graphics() {
    if (m_logicalDevice)
        m_logicalDevice->waitIdle();
}

void Graphics::createInstanceAndSurface() {
    auto layers = std::vector<const char *>();

    // Get required extensions for presenting to a window
    auto extensions = glfw::getRequiredInstanceExtensions();

#ifdef ENABLE_VALIDATION
    // Optionally enable validation layer and debug callback extension
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back("VK_EXT_debug_utils");
#endif

    // Create instance with the collected extensions and layers
    m_instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo()
            .setPEnabledLayerNames(layers)
            .setPEnabledExtensionNames(extensions)
    );

#ifdef ENABLE_VALIDATION
    // Optionally create a dynamic dispatch for the debug extension and a messenger to the callback function
    m_dispatch = vk::DispatchLoaderDynamic(*m_instance, glfw::getInstanceProcAddress);
    m_messenger = m_instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
            .setPfnUserCallback(Graphics::debugCallback),
        nullptr,
        m_dispatch
    );
#endif

    // Create surface for presenting
    m_surface = vk::UniqueSurfaceKHR(m_window.createSurface(*m_instance), *m_instance);
}

SpirvCode Graphics::loadAndCompileShader(shaderc::Compiler &compiler, shaderc_shader_kind kind, const char *path) {
    std::vector<char> glslSource;
    {
        // Open the file as a stream and seek to its end
        std::ifstream stream(path, std::ios::ate | std::ios::binary);
        if (!stream.is_open())
            throw std::runtime_error("Unable to open shader source");

        // Use the end position as the buffer length and seek back to the start
        auto length = stream.tellg();
        stream.seekg(0);
        glslSource.resize(static_cast<size_t>(length));

        // Read the file into the buffer
        stream.read(glslSource.data(), length);
        if (!stream.good())
            throw std::runtime_error("Unable to read shader source");
    }

    // Compile the GLSL shader to SPIR-V and optimize for performance
    auto options = shaderc::CompileOptions();
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    auto result = compiler.CompileGlslToSpv(glslSource.data(), glslSource.size(), kind, "glsl_shader", "main", options);

    // Check for errors and return the SPIR-V code if successful
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        throw std::runtime_error("Unable to compile shader: " + result.GetErrorMessage());
    return SpirvCode(result.cbegin(), result.cend());
}

void Graphics::loadAndCompileShaders() {
    auto compiler = shaderc::Compiler();

    m_vertexShaderCode = loadAndCompileShader(compiler, shaderc_vertex_shader, "triangle.vert");
    m_fragmentShaderCode = loadAndCompileShader(compiler, shaderc_fragment_shader, "triangle.frag");
}

#ifdef ENABLE_VALIDATION
VKAPI_ATTR VkBool32 VKAPI_CALL Graphics::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData
) {
    std::cerr << "[VK_EXT_debug_utils] " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
#endif
