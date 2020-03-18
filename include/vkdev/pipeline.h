#pragma once

#include "vkdev/device.h"
#include "vkdev/mesh.h"
#include "vkdev/rendertarget.h"
#include "vkdev/shader.h"

#include <memory>

namespace vkdev {

class Pipeline {
public:
    explicit Pipeline(Device& device_) : device(device_) {}

    VkPipeline handle = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    void cleanup();

private:
    Device& device;
};

std::unique_ptr<Pipeline> createDefaultPipeline(Device& device, Shader& shader, MeshDescription& meshDescription, SwapChainRenderTarget& renderTarget);

}
