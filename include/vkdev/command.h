#pragma once

#include "vkdev/queue.h"

#include <vulkan/vulkan.h>

namespace vkdev {

class SingleUseCommandBuffer;

// Note that command pools are tied to a specific queue.
class CommandPool {
public:
    CommandPool(VkPhysicalDevice physicalDevice_, VkDevice device_, const Queue& queue_)
        : physicalDevice(physicalDevice_), device(device_), queue(queue_) {}

    VkCommandPool handle = VK_NULL_HANDLE;
    const Queue& queue;

    SingleUseCommandBuffer createSingleUseBuffer();

    void create();
    void cleanup();

private:
    VkPhysicalDevice physicalDevice;
    VkDevice device;
};

class SingleUseCommandBuffer {
public:
    SingleUseCommandBuffer(const CommandPool& pool_, VkDevice device_): pool(pool_), device(device_) {}

    void start();
    void submit();

    inline bool isStarted() const { return handle != VK_NULL_HANDLE; }

    VkCommandBuffer handle = VK_NULL_HANDLE;

private:
    const CommandPool& pool;
    VkDevice device;
};

}