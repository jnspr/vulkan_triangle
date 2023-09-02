#include "graphics.hpp"

void Graphics::renderFrame() {
    // Wait for the next frame
    vk::resultCheck(
        m_logicalDevice->waitForFences(1, &m_nextFrameFence.get(), VK_TRUE, UINT64_MAX),
        "vk::Device::waitForFences"
    );
    vk::resultCheck(
        m_logicalDevice->resetFences(1, &m_nextFrameFence.get()),
        "vk::Device::resetFences"
    );

    // Acquire the next image for rendering
    uint32_t imageIndex;
    vk::resultCheck(
        m_logicalDevice->acquireNextImageKHR(*m_swapchain, UINT64_MAX, *m_imageAcquireSema, {}, &imageIndex),
        "vk::Device::acquireNextImageKHR",
        { vk::Result::eSuccess, vk::Result::eSuboptimalKHR }
    );

    // Record and submit the command buffer
    recordCommandBuffer(imageIndex);
    auto waitStage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    auto submitInfo = vk::SubmitInfo()
        .setPWaitSemaphores(&m_imageAcquireSema.get())
        .setPWaitDstStageMask(&waitStage)
        .setWaitSemaphoreCount(1)
        .setPCommandBuffers(&m_commandBuffer.get())
        .setCommandBufferCount(1)
        .setPSignalSemaphores(&m_renderFinishSema.get())
        .setSignalSemaphoreCount(1);
    vk::resultCheck(m_queue.submit(1, &submitInfo, *m_nextFrameFence), "vk::Queue::submit");

    // Queue presentation to occur when rendering is finished
    vk::resultCheck(
        m_queue.presentKHR(vk::PresentInfoKHR()
                               .setPWaitSemaphores(&m_renderFinishSema.get())
                               .setWaitSemaphoreCount(1)
                               .setPSwapchains(&m_swapchain.get())
                               .setSwapchainCount(1)
                               .setPImageIndices(&imageIndex)
        ),
        "vk::Queue::presentKHR",
        { vk::Result::eSuccess, vk::Result::eSuboptimalKHR }
    );
}

void Graphics::handleResize() {
    // Wait for pending operations to finish
    m_logicalDevice->waitIdle();

    // Destroy the graphics pipeline
    m_graphicsPipelineLayout.reset();
    m_graphicsPipeline.reset();

    // Destroy the swapchain and resources that depend on it
    m_framebuffers.clear();
    m_imageViews.clear();
    m_swapchain.reset();

    // Re-create the swapchain and resources that depend on it
    createSwapchain();
    createImageViews();
    createFramebuffers();

    // Re-create the graphics pipeline
    initViewportAndScissor();
    createGraphicsPipeline();
}

void Graphics::recordCommandBuffer(uint32_t imageIndex) {
    // Reset the buffer and start recording
    m_commandBuffer->reset();
    m_commandBuffer->begin(vk::CommandBufferBeginInfo());

    // Start the render pass with a solid black clear color
    auto clearValue = vk::ClearValue()
        .setColor({0.0f, 0.0f, 0.0f, 1.0f});
    m_commandBuffer->beginRenderPass(
        vk::RenderPassBeginInfo()
            .setRenderPass(*m_renderPass)
            .setFramebuffer(*m_framebuffers[imageIndex])
            .setRenderArea(m_scissor)
            .setPClearValues(&clearValue)
            .setClearValueCount(1),
        vk::SubpassContents::eInline
    );

    // Bind the graphics pipeline
    m_commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);

    // Set viewport and scissor
    m_commandBuffer->setViewport(0, 1, &m_viewport);
    m_commandBuffer->setScissor(0, 1, &m_scissor);

    // Draw the triangle
    m_commandBuffer->bindVertexBuffers(0, {*m_vertexBuffer}, {0});
    m_commandBuffer->draw(k_vertexData.size(), 1, 0, 0);

    // End the render pass and recording
    m_commandBuffer->endRenderPass();
    m_commandBuffer->end();
}
