#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace vkdev {
    struct UniformBuffer {
        UniformBuffer(uint32_t size_): size(size) {}

        uint32_t size;
        std::vector<VkBuffer> buffers;
        std::vector< VkDeviceMemory > memory;
    };

    class DescriptorPool {
    public:
        DescriptorPool(VkPhysicalDevice physicalDevice_, VkDevice device_, uint32_t descriptorCount_) : 
            physicalDevice(physicalDevice_), device(device_), descriptorCount(descriptorCount_) {};

        void addDescriptor(VkDescriptorType type, VkShaderStageFlagBits stage);
        void addUniformBuffer(const std::string& name, uint32_t size, VkShaderStageFlagBits stage);

        void create();
        void cleanup();

        std::vector<VkDescriptorPoolSize> descriptorSizes;
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        std::map< std::string, std::unique_ptr<UniformBuffer>> uniformBuffers;

        VkDescriptorPool poolHandle;
        VkDescriptorSetLayout layoutHandle;

    private:
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        uint32_t descriptorCount;
    };
}