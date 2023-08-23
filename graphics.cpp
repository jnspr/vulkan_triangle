#include "graphics.hpp"

#include <stdexcept>

Graphics::Graphics(glfw::Window &window): m_queueFamilyIndex(0xffffffff) {
    // Create instance with extensions for presenting to window surface
    auto extensions = glfw::getRequiredInstanceExtensions();
    m_instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo()
            .setPEnabledExtensionNames(extensions)
    );

    // Create surface for presenting and get its current size
    m_surface = vk::UniqueSurfaceKHR(window.createSurface(*m_instance), *m_instance);
    auto size = window.getFramebufferSize();

    // Setup devices and presentation
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapchain(vk::Extent2D(std::get<0>(size), std::get<1>(size)));
}

void Graphics::selectPhysicalDevice() {
    for (auto device : m_instance->enumeratePhysicalDevices()) {
        // Only use non-CPU devices
        auto properties = device.getProperties();
        if (properties.deviceType == vk::PhysicalDeviceType::eCpu)
            continue;

        // Only use devices where `m_surface` has at least one format
        auto formats = device.getSurfaceFormatsKHR(*m_surface);
        if (formats.empty())
            continue;

        auto queueFamilies = device.getQueueFamilyProperties();
        for (uint32_t index = 0; index < queueFamilies.size(); index++) {
            // Only use queue families which support graphics operations
            if (!(queueFamilies[index].queueFlags & vk::QueueFlagBits::eGraphics))
                continue;

            // Only use queue families which support graphics and present operations on `m_surface`
            if (!device.getSurfaceSupportKHR(index, *m_surface))
                continue;
            if (!glfw::getPhysicalDevicePresentationSupport(*m_instance, device, index))
                continue;

            m_physicalDevice = device;
            m_surfaceFormat = formats[0];
            m_queueFamilyIndex = index;
            return;
        }
    }
    throw std::runtime_error("No supported physical device was found");
}

void Graphics::createLogicalDevice() {
    // Define a single queue (the graphics queue) that has to be created with the device
    const float queuePriority = 1.0f;
    auto queueCreateInfo = vk::DeviceQueueCreateInfo()
        .setQueueFamilyIndex(m_queueFamilyIndex)
        .setPQueuePriorities(&queuePriority)
        .setQueueCount(1);

    // Create a logical device with swapchain support
    static const char *swapchainExtension = "VK_KHR_swapchain";
    m_logicalDevice = m_physicalDevice.createDeviceUnique(
        vk::DeviceCreateInfo()
            .setPpEnabledExtensionNames(&swapchainExtension)
            .setEnabledExtensionCount(1)
            .setPQueueCreateInfos(&queueCreateInfo)
            .setQueueCreateInfoCount(1)
    );

    // Obtain the created queue's handle
    m_queue = m_logicalDevice->getQueue(m_queueFamilyIndex, 0);
}

void Graphics::createSwapchain(vk::Extent2D imageExtent) {
    auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);

    // Check if the image extent is within the allowed range
    if (imageExtent.width  < capabilities.minImageExtent.width  ||
        imageExtent.height < capabilities.minImageExtent.height ||
        imageExtent.width  > capabilities.maxImageExtent.width  ||
        imageExtent.height > capabilities.maxImageExtent.height)
    {
        throw std::runtime_error("Unable create swapchain with given image extent");
    }

    uint32_t minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount != 0 && minImageCount > capabilities.maxImageCount)
        minImageCount = capabilities.maxImageCount;

    m_swapchain = m_logicalDevice->createSwapchainKHR(
        vk::SwapchainCreateInfoKHR()
            // Present to `m_surface` using a FIFO
            .setSurface(*m_surface)
            .setPresentMode(vk::PresentModeKHR::eFifo)
            // Use the previously selected graphics queue family
            .setPQueueFamilyIndices(&m_queueFamilyIndex)
            .setQueueFamilyIndexCount(1)
            // Swap through `minImageCount` images with color attachment
            .setImageFormat(m_surfaceFormat.format)
            .setImageColorSpace(m_surfaceFormat.colorSpace)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageExtent(imageExtent)
            .setImageArrayLayers(1)
            .setMinImageCount(minImageCount)
    );
}
