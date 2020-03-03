#include "vkdev/command.h"

#include <stdexcept>
#include <vkdev/queue.h>

namespace vkdev {

void CommandPool::create() {
    auto queueFamilyIndicies = queue::findFamilies(physicalDevice, surface);

    // command pool can only create commands on a particular type of queue.  In or case we are making graphics commands so needs to be associated with our graphics queue
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily.value();
    poolInfo.flags = 0;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }
}

void CommandPool::cleanup(){
    vkDestroyCommandPool(device, handle, nullptr);
}

SingleUseCommandBuffer CommandPool::createSingleUseBuffer(){
    return SingleUseCommandBuffer(handle, device);
}

void SingleUseCommandBuffer::start() {
    VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandPool = pool;
    commandBufferAllocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &handle);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(handle, &beginInfo);
}

void SingleUseCommandBuffer::submit(VkQueue queue) {
    vkEndCommandBuffer(handle);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &handle;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &handle);
}

}