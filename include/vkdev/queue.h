#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>

namespace vkdev::queue {

struct FamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentationFamily; // allows for displaying to a surface

    inline bool isComplete() const { return graphicsFamily.has_value() && presentationFamily.has_value(); }
};

// todo break this up into multiple functions?
FamilyIndices findFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

}
