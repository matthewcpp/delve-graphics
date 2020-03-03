#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace vkdev {
    class SwapChain {
    public:
        SwapChain(VkPhysicalDevice physicalDevice_, VkDevice device_, VkSurfaceKHR surface_) :
            physicalDevice(physicalDevice_), device(device_), surface(surface_) {}

        void create(const glm::ivec2& framebufferSize);
        void cleanup();
    public:
        VkSwapchainKHR handle;
        VkFormat imageFormat;
        VkExtent2D extent;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;

    private:
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkSurfaceKHR surface;
    };
}
