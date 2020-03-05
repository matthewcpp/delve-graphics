#pragma once

#include <vulkan/vulkan.h>

namespace vkdev {
    class Instance {
    public:
        VkInstance handle = VK_NULL_HANDLE;

        void create(bool enableValidationLayers);
        void cleanup();
    };
}