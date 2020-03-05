#include "vkdev/buffer.h"

#include <stdexcept>

namespace vkdev {
    // search though all the available memory types to find the index of correct source
    // note that the memory type must be a match as well as the required properties (such as write access, etc)
    uint32_t Buffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void Buffer::create(VkPhysicalDevice physicalDevice_, VkDevice device_, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        physicalDevice = physicalDevice_;
        device = device_;

        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = bufferSize; // buffer size in bytes
        vertexBufferInfo.usage = usage;
        vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &vertexBufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        // describe the memory we want to allocate into the buffer
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory.");
        }

        //now that memory is allocated, associate it with vertex buffer we just made above
        if (vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
            throw std::runtime_error("failed to bind memory to buffer");
        }

        size = bufferSize;
    }

    void Buffer::createWithData(VkPhysicalDevice physicalDevice_, VkDevice device_, void* data, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        create(physicalDevice_, device_, size, usage, properties);

        void* mappedData = nullptr;
        vkMapMemory(device, memory, 0, size, 0, &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(size));
        vkUnmapMemory(device, memory);
    }

    // copying a vertex buffer requires a transfer command.  We will need to create a temporary command buffer to execute the command
    // Ideally it would be useful to create a separate command pool for short lived transfer operations like this as opposed to using the main command pool.
    // note that we are using the graphics queue to perform copies.  this is because graphics queues must also support buffer copy operations.
    void Buffer::copy(CommandPool& commandPool, const Buffer& src, Buffer& dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size) {
        if (size == std::numeric_limits<VkDeviceSize>::max()) {
            size = src.size;
        }

        auto commandBuffer = commandPool.createSingleUseBuffer();
        commandBuffer.start();

        VkBufferCopy regionToCopy = {}; // copy the whole buffer in one shot
        regionToCopy.srcOffset = srcOffset;
        regionToCopy.dstOffset = dstOffset;
        regionToCopy.size = size;
        vkCmdCopyBuffer(commandBuffer.handle, src.buffer, dst.buffer, 1, &regionToCopy);

        commandBuffer.submit();
    }

    void Buffer::cleanup() {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
}