#include "vkdev/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace vkdev::Texture {

Image createFromFile(const std::string& path, Device& device, CommandPool& commandPool) {
    Image textureImage{ device };

    int width, height, numChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &numChannels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("failed to load texture image.");
    }

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    // we follow the convention of creating the staging buffer, mapping memory then transferring to destination buffer
    VkDeviceSize imageSize = width * height * 4;
    vkdev::Buffer stagingBuffer{ device };

    stagingBuffer.createWithData(pixels, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY);

    stbi_image_free(pixels);

    // note that since we are generating mipmaps via vkCmdBlitImage we need to inform vulkan that image buffer will be both a source and destination of image operations
    const VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    textureImage.create(static_cast<uint32_t>(width), static_cast<uint32_t>(height), mipLevels, VK_SAMPLE_COUNT_1_BIT, imageFormat, VK_IMAGE_TILING_OPTIMAL, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // note: image was created with undefined layout in createImage function above we will transition the image into a state where it can have the data loaded into it.
    textureImage.transitionLayout(commandPool, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    textureImage.loadBufferData(commandPool, stagingBuffer);

    if (textureImage.mipLevels > 1) {
        // note that this function will transition all mipmap levels to optimal read format.  in lieu of this function, mipmap levels could be loaded in manually
        textureImage.generateMipmaps(commandPool);
    }
    else {
        // now that the data is copied into the image, transition it to optimal read format
        textureImage.transitionLayout(commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    textureImage.createView(VK_IMAGE_ASPECT_COLOR_BIT);

    stagingBuffer.cleanup();

    return textureImage;
}

}
