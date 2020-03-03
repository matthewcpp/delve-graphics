#pragma once

#include <vulkan/vulkan.h>

namespace vkdev {

class SingleUseCommandBuffer;

class CommandPool {
public:
    CommandPool(VkPhysicalDevice physicalDevice_, VkDevice device_, VkSurfaceKHR surface_)
        : physicalDevice(physicalDevice_), device(device_), surface(surface_) {}

    VkCommandPool handle = VK_NULL_HANDLE;

    SingleUseCommandBuffer createSingleUseBuffer();

    void create();
    void cleanup();

private:
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
};

class SingleUseCommandBuffer {
public:
    SingleUseCommandBuffer(VkCommandPool pool_, VkDevice device_): pool(pool_), device(device_) {}

    void start();
    void submit(VkQueue queue);

    inline bool isStarted() const { return handle != VK_NULL_HANDLE; }

    VkCommandBuffer handle = VK_NULL_HANDLE;

private:
    VkCommandPool pool;
    VkDevice device;
};

}