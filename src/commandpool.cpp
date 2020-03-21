#include "vkdev/commandpool.h"

#include <stdexcept>
#include <vkdev/queue.h>

namespace vkdev {

void CommandPool::create() {

    // command pool can only create commands on a particular type of queue.  In or case we are making graphics commands so needs to be associated with our graphics queue
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queue.index;
    poolInfo.flags = 0;

    if (vkCreateCommandPool(device.logical, &poolInfo, nullptr, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }
}

void CommandPool::cleanup(){
    vkDestroyCommandPool(device.logical, handle, nullptr);
}

SingleUseCommandBuffer CommandPool::createSingleUseBuffer(){
    return SingleUseCommandBuffer(*this, device);
}

void SingleUseCommandBuffer::start() {
    VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandPool = pool.handle;
    commandBufferAllocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device.logical, &commandBufferAllocInfo, &handle);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(handle, &beginInfo);
}

void SingleUseCommandBuffer::submit() {
    vkEndCommandBuffer(handle);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &handle;

    vkQueueSubmit(pool.queue.handle, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pool.queue.handle);

    vkFreeCommandBuffers(device.logical, pool.handle, 1, &handle);
}

}