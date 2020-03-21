#pragma once

#include "vkdev/queue.h"
#include "vkdev/device.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace vkdev {
    class SwapChain {
    public:
        SwapChain(Device& device_, VkSurfaceKHR surface_): device(device_), surface(surface_) {}

        void create(const glm::ivec2& framebufferSize);
        void cleanupImages();
        void cleanupSyncObjects();

        VkResult aquireFrame(uint32_t& index);
        VkResult drawFrame(uint32_t imageIndex, VkCommandBuffer commandBuffer);

        void createSyncObjects();

        static const int MAX_SIMULTANEOUS_FRAMES = 2;
    public:
        VkSwapchainKHR handle;
        VkFormat imageFormat;
        VkExtent2D extent;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> inFlightImages;
        size_t currentFrameIndex = 0;

    private:
        Device& device;
        VkSurfaceKHR surface;
    };

    struct SwapChainSupportInfo {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;

        static SwapChainSupportInfo getForDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    };
}
