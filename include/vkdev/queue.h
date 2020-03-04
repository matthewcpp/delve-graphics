#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <limits>

namespace vkdev {

struct Queue {
    VkQueue handle = VK_NULL_HANDLE;
    uint32_t index = std::numeric_limits<uint32_t>::max();

    static uint32_t findGraphicsQueueIndex(VkPhysicalDevice physicalDevice);
    static uint32_t findPresentationQueueIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
};

}
