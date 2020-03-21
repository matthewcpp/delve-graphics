#pragma once

#include "vkdev/commandpool.h"
#include "vkdev/descriptor.h"
#include "vkdev/device.h"
#include "vkdev/mesh.h"
#include "vkdev/pipeline.h"
#include "vkdev/rendertarget.h"

#include <vector>

namespace vkdev{

class RenderCommand{
public:
    RenderCommand(Device& device_, CommandPool& commandPool_): device(device_), commandPool(commandPool_) {}

    void create(SwapChainRenderTarget& renderTarget, Pipeline& piepline, Mesh& mesh, Descriptor& descriptor);
    void cleanup();

    std::vector<VkCommandBuffer> commandBuffers;
private:
    Device& device;
    CommandPool& commandPool;
};

}
