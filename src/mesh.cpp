#include "vkdev/mesh.h"

#include <fstream>
#include <stdexcept>

namespace vkdev {

void MeshData::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Unable to load file: " + path);
    }

    file.read(reinterpret_cast<char*>(&vertexAttributes), sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(&vertexCount), sizeof(uint32_t));

    uint32_t vertexBufferSize = 0;
    file.read(reinterpret_cast<char*>(&vertexBufferSize), sizeof(uint32_t));

    vertexBuffer.resize(vertexBufferSize);
    file.read(reinterpret_cast<char*>(vertexBuffer.data()), vertexBufferSize);

    file.read(reinterpret_cast<char*>(&elementCount), sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(&elementSize), sizeof(uint32_t));

    uint32_t elementBufferSize = 0;
    file.read(reinterpret_cast<char*>(&elementBufferSize), sizeof(uint32_t));

    elementBuffer.resize(elementBufferSize);
    file.read(reinterpret_cast<char*>(elementBuffer.data()), elementBufferSize);

    file.read(reinterpret_cast<char*>(&bounds), elementBufferSize);
}

/*
Loading a model requires the creation of a vertex and index buffer.  Creating these buffers involves the following steps:
create a temporary CPU visible staging buffer to copy vertex data to the GPU (device local)
create our vertex buffer which will hold data on the GPU
copy the staging buffer to device local buffer
cleanup staging buffer
*/
void Mesh::create(const MeshData& meshData, CommandPool& commandPool) {
    const VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(meshData.vertexBuffer.size());

    vkdev::Buffer stagingBuffer{ device };
    stagingBuffer.createWithData(meshData.vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY);
    vertexBuffer.create(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY );
    vkdev::Buffer::copy(commandPool, stagingBuffer, vertexBuffer);
    stagingBuffer.cleanup();

    const VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(meshData.elementBuffer.size());
    stagingBuffer.createWithData(meshData.elementBuffer.data(), indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY);
    indexBuffer.create(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY);
    vkdev::Buffer::copy(commandPool, stagingBuffer, indexBuffer);
    stagingBuffer.cleanup();

    vertexAttributes = meshData.vertexAttributes;
    vertexCount = meshData.vertexCount;
    elementCount = meshData.elementCount;
    elementSize = meshData.elementSize;
    bounds = meshData.bounds;
}

uint32_t Mesh::vertexSize() const {
    uint32_t size = 0;

    if (vertexAttributes & MeshVertexAttributes::Positions)
        size += 3 * sizeof(float);

    if (vertexAttributes & MeshVertexAttributes::Normals)
        size += 3 * sizeof(float);

    if (vertexAttributes & MeshVertexAttributes::TexCoords)
        size += 2 * sizeof(float);

    return size;
}

MeshDescription Mesh::getMeshDescription() const {
    MeshDescription description;
    description.bindingDescription = getBindingDescription();
    description.attributeDescriptions = getAttributeDescriptions();

    return description;
}

VkVertexInputBindingDescription Mesh::getBindingDescription() const {
    // describes the format of the vertex
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;  // describes position in array of bindings
    bindingDescription.stride = vertexSize();
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Mesh::getAttributeDescriptions() const {
    std::vector<VkVertexInputAttributeDescription> descriptions;
    uint32_t offset = 0U;

    if (vertexAttributes & MeshVertexAttributes::Positions) {
        VkVertexInputAttributeDescription desc = {};
        desc.binding = 0;
        desc.location = static_cast<uint32_t>(descriptions.size()); //location in the shader ie. layout(LOCATION = 0) etc
        desc.format = VK_FORMAT_R32G32B32_SFLOAT;
        desc.offset = offset;

        descriptions.push_back(desc);
        offset += 3 * sizeof(float);
    }

    if (vertexAttributes & MeshVertexAttributes::Normals) {
        VkVertexInputAttributeDescription desc = {};
        desc.binding = 0;
        desc.location = static_cast<uint32_t>(descriptions.size());
        desc.format = VK_FORMAT_R32G32B32_SFLOAT;
        desc.offset = offset;

        descriptions.push_back(desc);
        offset += 3 * sizeof(float);
    }

    if (vertexAttributes & MeshVertexAttributes::TexCoords) {
        VkVertexInputAttributeDescription desc = {};
        desc.binding = 0;
        desc.location = static_cast<uint32_t>(descriptions.size());
        desc.format = VK_FORMAT_R32G32_SFLOAT;
        desc.offset = offset;

        descriptions.push_back(desc);
        offset += 2 * sizeof(float);
    }

    return descriptions;
}

void Mesh::cleanup() {
    indexBuffer.cleanup();
    vertexBuffer.cleanup();
}

}