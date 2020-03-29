#include "vkdev/descriptor.h"

#include <stdexcept>

namespace vkdev {

void Descriptor::createPool(const Shader& shader, uint32_t count){
    std::vector<VkDescriptorPoolSize> poolSizes(shader.info.uniforms.size());

    for (size_t i = 0; i < shader.info.uniforms.size(); i++) {
        poolSizes[i].type = shader.info.uniforms[i].type;
        poolSizes[i].descriptorCount = count;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = count;

    if (vkCreateDescriptorPool(device.logical, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool.");
    }
}

// texture sampler object will describe how we will sample the texture from within our shader.
VkSampler createTextureSampler(Device& device, uint32_t mipLevels) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    //address mode is described per axis...describes how to deal with reading texels outside the image
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // note that for this to function correctly this needs to be enabled in physical device features struct when picking physical device
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;

    // color to use when sampling beyond texel range with clamp to border enabled.
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE; // if true then you would address texels by [0, width) and [0, height)

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // mipmapping settings
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device.logical, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler.");
    }

    return sampler;
}

void Descriptor::createDescriptorSets(Material& material, Assets& assets, uint32_t count, uint32_t mipLevels) {
    Shader& shader = *(assets.shaders[material.shader]);

    std::vector<VkDescriptorSetLayout> layouts(count, shader.descriptorLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(count);
    if (vkAllocateDescriptorSets(device.logical, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }


    for (size_t ds = 0; ds < descriptorSets.size(); ds++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.reserve(shader.info.getUniformTypeCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

        std::vector< VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(shader.info.getUniformTypeCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER));

        for (size_t i = 0; i < shader.info.uniforms.size(); i++) {
            const Uniform& uniform = shader.info.uniforms[i];

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[ds];
            descriptorWrite.dstBinding = static_cast<uint32_t>(i);
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.descriptorType = uniform.type;

            if (uniform.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                auto& uniformBufferVector = uniformBuffers[uniform.name];
                uniformBufferVector.reserve(count);

                // create the actual backing buffers that will hold the data accessed by the shader
                for (uint32_t i = 0; i < count; i++) {
                    auto& uniformBuffer = uniformBufferVector.emplace_back(device);
                    uniformBuffer.create(uniform.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY);
                }

                // create the bufferInfo struct for this buffer
                VkDescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = uniformBufferVector[i].buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = static_cast<VkDeviceSize>(uniform.size);
                bufferInfos.push_back(bufferInfo);

                descriptorWrite.pBufferInfo = &bufferInfos.back();
                descriptorWrite.pImageInfo = nullptr;
            }
            else if (uniform.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                VkSampler sampler = [&](){
                    auto result = samplers.find(uniform.name);
                    if (result == samplers.end()) {
                        VkSampler sampler = createTextureSampler(device, mipLevels);
                        samplers[uniform.name] = sampler;
                        return sampler;
                    }
                    else {
                        return result->second;
                    }
                }();

                // create the imageinfo struct for this sampler
                VkDescriptorImageInfo imageInfo = {};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                auto textureImage = material.textures.find(uniform.name);

                imageInfo.imageView = textureImage->second->view;
                imageInfo.sampler = sampler;
                imageInfos.push_back(imageInfo);

                descriptorWrite.pImageInfo = &imageInfos.back();
                descriptorWrite.pBufferInfo = nullptr;
            }

            descriptorWrite.pTexelBufferView = nullptr;
            descriptorWrite.pNext = nullptr;

            descriptorWrites.push_back(descriptorWrite);
        }

        vkUpdateDescriptorSets(device.logical, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

    }


}

void Descriptor::create(Material& material, Assets& assets, uint32_t count, uint32_t mipLevels) {
    auto shader = assets.shaders.find(material.shader);

    if (shader != assets.shaders.end()) {
        createPool(*shader->second, count);
        createDescriptorSets(material, assets, count, mipLevels);
    }
    else {
        throw std::runtime_error("Could not create descriptor.  Unknown shader: " + material.shader);
    }
}

void Descriptor::cleanup() {
    vkDestroyDescriptorPool(device.logical, pool, nullptr);

    for (auto& uniformBufferVector : uniformBuffers) {
        for (size_t i = 0; i < uniformBufferVector.second.size(); i++) {
            uniformBufferVector.second[i].cleanup();
        }
    }

    uniformBuffers.clear();

    for (auto& sampler : samplers) {
        vkDestroySampler(device.logical, sampler.second, nullptr);
    }

    samplers.clear();
}

}
