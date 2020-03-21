#pragma once

#include "vkdev/buffer.h"
#include "vkdev/commandpool.h"
#include "vkdev/device.h"

#include <vulkan/vulkan.h>

namespace vkdev {
    
class Image {
public:
    explicit Image(Device& device_): device(device_) {}

    VkImage handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;

    void create(uint32_t width_, uint32_t height_, uint32_t mipLevels_, VkSampleCountFlagBits numSamples, VkFormat format_, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
    void transitionLayout(CommandPool& commandPool, VkImageLayout oldLayout, VkImageLayout newLayout);
    void loadBufferData(CommandPool& commandPool, const Buffer& buffer);
    void generateMipmaps(CommandPool& commandPool);
    void createView(VkImageAspectFlags aspectFlags);

    void cleanup();

    static VkImageView createView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    static VkFormat findSupportedFormat(Device& device, const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags features);
    static inline bool formatHasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }

private:
    Device& device;
};

}
