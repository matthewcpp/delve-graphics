#pragma once

#include "vkdev/device.h"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>


namespace vkdev {

class ShaderData {
public:
    void loadFiles(const std::string& vertexFilePath, const std::string& fragmentFilePath);

    std::vector<char> vertexShaderCode;
    std::vector<char> fragmentShaderCode;
};

class Shader {
public:
    explicit Shader(Device& device_) : device(device_) {}

    void create(const ShaderData& data);
    void cleanup();

    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

private:
    Device& device;
};

}
