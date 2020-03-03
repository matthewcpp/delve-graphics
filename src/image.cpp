#include "vkdev/image.h"

#include <stdexcept>

namespace vkdev::image {

VkImageView createView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageViewHandle = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageViewHandle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view.");
    }

    return imageViewHandle;
}

}
