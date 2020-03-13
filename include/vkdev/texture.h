#pragma once

#include "vkdev/command.h"
#include "vkdev/device.h"
#include "vkdev/image.h"

#include <string>

namespace vkdev::Texture {
    Image createFromFile(const std::string& path, Device& device, CommandPool& commandPool);
}
