#pragma once

#include "vkdev/buffer.h"
#include "vkdev/commandpool.h"
#include "vkdev/device.h"
#include "vkdev/bounds.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace vkdev {

enum MeshVertexAttributes: uint32_t {
    Unset = 0,
    Positions = 1,
    Normals = 2,
    TexCoords = 4
};

struct MeshDescription {
    VkVertexInputBindingDescription bindingDescription;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};

struct MeshData {
    MeshVertexAttributes vertexAttributes;
    std::vector<uint8_t> vertexBuffer;
    uint32_t vertexCount;

    std::vector<uint8_t> elementBuffer;
    uint32_t elementCount;
    uint32_t elementSize;

    Bounds bounds;

    void loadFromFile(const std::string& path);
};

class Mesh {
public:
    explicit Mesh(Device& device_) : vertexBuffer(device_), indexBuffer(device_), device(device_) {}

    void cleanup();
    void create(const MeshData& meshData, CommandPool& commandPool);

    uint32_t vertexSize() const;
    MeshDescription getMeshDescription() const;

    Buffer vertexBuffer;
    Buffer indexBuffer;

    MeshVertexAttributes vertexAttributes = MeshVertexAttributes::Unset;
    uint32_t vertexCount = 0;
    uint32_t elementCount = 0;
    uint32_t elementSize = 0;

    Bounds bounds;

private:
    VkVertexInputBindingDescription getBindingDescription() const;
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() const;

private:
    Device device;
};

}