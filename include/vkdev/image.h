#pragma once

#include <vulkan/vulkan.h>

namespace vkdev::image {
    VkImageView createView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
}
