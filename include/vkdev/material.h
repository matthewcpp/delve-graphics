#pragma once

#include "vkdev/image.h"

#include <unordered_map>

namespace vkdev {

struct Material {
    std::string shader;
    std::unordered_map<std::string, vkdev::Image*> textures;
};

}
