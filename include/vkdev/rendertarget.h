#pragma once

#include "vkdev/assets.h"
#include "vkdev/command.h"
#include "vkdev/device.h"
#include "vkdev/swapchain.h"

#include <vector>

namespace vkdev {

/**
Render Target definition:
VkFramebuffer + VkRenderPass defines the render target.
Render pass defines which attachment will be written with colors.
VkFramebuffer defines which VkImageView is to be which attachment.
VkImageView defines which part of VkImage to use.
VkImage defines which VkMemory is used and a format of the texel
*/
class SwapChainRenderTarget {
public:
    explicit SwapChainRenderTarget(Device& device_) : device(device_) {}

    void create(SwapChain& swapchain, CommandPool& commandPool);

    void cleanup();

    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;

    VkSampleCountFlagBits msaaSampleCount = VK_SAMPLE_COUNT_4_BIT;

private:
    void createImages(SwapChain& swapchain, CommandPool& commandPool, VkFormat depthFormat);
    void createRenderPass(SwapChain& swapchain, VkFormat depthFormat);
    void createFramebuffers(SwapChain& swapchain);

private:
    Device& device;

    std::unique_ptr<vkdev::Image> depthImage;
    std::unique_ptr<vkdev::Image> msaaColorImage;
};

}