#include "vkdev/shader.h"

#include <fstream>
#include <stdexcept>

namespace vkdev {

std::vector<char> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("unable to read file: " + path);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    return buffer;
}

void ShaderData::loadFiles(const std::string& vertexFilePath, const std::string& fragmentFilePath) {
    vertexShaderCode = readFile(vertexFilePath);
    fragmentShaderCode = readFile(fragmentFilePath);
}


VkShaderModule createShaderModule(const std::vector<char>& code, Device& device) {
    VkShaderModuleCreateInfo shaderInfo = {};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = code.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;

    if (vkCreateShaderModule(device.logical, &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }

    return shaderModule;
}

void Shader::create(const ShaderData& data) {
    vertexShader = createShaderModule(data.vertexShaderCode, device);
    fragmentShader = createShaderModule(data.fragmentShaderCode, device);
}

void Shader::cleanup() {
    vkDestroyShaderModule(device.logical, vertexShader, nullptr);
    vkDestroyShaderModule(device.logical, fragmentShader, nullptr);
}

}