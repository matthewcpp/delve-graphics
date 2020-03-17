#pragma once

#include "vkdev/mesh.h"
#include "vkdev/shader.h"
#include "vkdev/texture.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace vkdev {

struct Assets {
    std::unordered_map<std::string, std::unique_ptr<vkdev::Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<vkdev::Image>> textures;
    std::unordered_map<std::string, std::unique_ptr<vkdev::Shader>> shaders;

    void cleanup();
};

}