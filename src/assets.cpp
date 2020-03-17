#include "vkdev/assets.h"

namespace vkdev {

void Assets::cleanup() {
    for (auto& mesh : meshes) {
        mesh.second->cleanup();
    }

    for (auto& texture : textures) {
        texture.second->cleanup();
    }

    for (auto& shader : shaders) {
        shader.second->cleanup();
    }
}

}