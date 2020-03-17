#pragma once

#include "vkdev/assets.h"
#include "vkdev/buffer.h"
#include "vkdev/device.h"
#include "vkdev/material.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace vkdev {

class Descriptor{
public:
    explicit Descriptor(Device& device_): device(device_) {}
    void create(Material& material, Assets& assets, uint32_t count, uint32_t mipLevels);
    void cleanup();

    VkDescriptorPool pool = VK_NULL_HANDLE;

    std::vector<VkDescriptorSet> descriptorSets;

    std::unordered_map<std::string, std::vector<vkdev::Buffer>> uniformBuffers;
    std::unordered_map<std::string, VkSampler> samplers;

private:
    void createPool(const Shader& shaderInfo, uint32_t count);
    void createDescriptorSets(Material& material, Assets& assets, uint32_t count, uint32_t mipLevels);

private:
    Device& device;
};

}
