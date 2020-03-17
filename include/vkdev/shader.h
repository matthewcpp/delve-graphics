#pragma once

#include "vkdev/device.h"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>


namespace vkdev {

struct Uniform {
    std::string name;
    VkDescriptorType type;
    VkShaderStageFlagBits stage;
    uint32_t size;
};

class ShaderInfo {
public:
    std::vector<std::string> attributes;
    std::vector<Uniform> uniforms;

    size_t getUniformTypeCount(VkDescriptorType type) const;
};

class ShaderData {
public:
    void loadFiles(const std::string& vertexFilePath, const std::string& fragmentFilePath, const std::string infoFilePath);

    std::vector<char> vertexShaderCode;
    std::vector<char> fragmentShaderCode;
    std::string infoJson;
};

class Shader {
public:
    explicit Shader(Device& device_) : device(device_) {}

    void create(const ShaderData& data);
    void cleanup();

    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    ShaderInfo info;

    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

private:
    Device& device;
};

}
