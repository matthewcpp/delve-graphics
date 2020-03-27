#pragma once

#include "vkdev/commandpool.h"
#include "vkdev/device.h"

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <limits>


namespace vkdev {
    class Buffer {
    public:
        explicit Buffer(Device& device_): device(device_) {}

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation  allocation;
        VkDeviceSize size = 0;

        void cleanup();

        void create(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
        void createWithData(const void* data, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

        static void copy(CommandPool& commandPool, const Buffer& src, Buffer& dst, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0, VkDeviceSize size = std::numeric_limits<VkDeviceSize>::max());
        static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    private:
        Device& device;
    };
}
