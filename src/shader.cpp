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

void ShaderData::loadFiles(const std::string& vertexFilePath, const std::string& fragmentFilePath, const std::string infoFilePath) {
    vertexShaderCode = readFile(vertexFilePath);
    fragmentShaderCode = readFile(fragmentFilePath);

    const auto infoFile = readFile(infoFilePath);
    infoJson.assign(infoFile.data(), infoFile.size());
}

size_t ShaderInfo::getUniformTypeCount(VkDescriptorType type) const {
    size_t count = 0;

    for (const auto& uniform : uniforms) {
        if (uniform.type == type) {
            count +=1;
        }
    }

    return count;
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

// We need to provide details about every descriptor binding used in the shaders for pipeline creation
// note that the descriptor set remains valid even when creating new pipelines.
VkDescriptorSetLayout createDescriptorSetLayout(Device& device, const ShaderInfo& info) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (size_t i = 0; i < info.uniforms.size(); i++){
        const auto& uniform = info.uniforms[i];

        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = static_cast<uint32_t>(i);
        layoutBinding.descriptorType = uniform.type;
        layoutBinding.descriptorCount = 1; // if an array, specifies the number of items in the array
        layoutBinding.stageFlags = uniform.stage;
        layoutBinding.pImmutableSamplers = nullptr; // used for image sampling

        bindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device.logical, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }

    return descriptorSetLayout;
}

void Shader::create(const ShaderData& data) {
    vertexShader = createShaderModule(data.vertexShaderCode, device);
    fragmentShader = createShaderModule(data.fragmentShaderCode, device);

    // temp
    info.uniforms.push_back({ "UniformBufferObject", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 192 });
    info.uniforms.push_back({ "texSampler", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0 });

    descriptorLayout = createDescriptorSetLayout(device, info);
}

void Shader::cleanup() {
    vkDestroyShaderModule(device.logical, vertexShader, nullptr);
    vkDestroyShaderModule(device.logical, fragmentShader, nullptr);

    vkDestroyDescriptorSetLayout(device.logical, descriptorLayout, nullptr);
}

}