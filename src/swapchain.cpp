#include "vkdev/swapchain.h"
#include "vkdev/image.h"

#include <optional>
#include <algorithm>
#include <stdexcept>

namespace vkdev {
    struct SwapChainSupportInfo {
        VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentationFamily; // allows for displaying to a surface

        [[nodiscard]] inline bool isComplete() const { return graphicsFamily.has_value() && presentationFamily.has_value(); }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;

                if (indices.isComplete()) {
                    break;
                }
            }

            VkBool32 presentationSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentationSupport);

            if (presentationSupport) {
                indices.presentationFamily = i;

                if (indices.isComplete()) {
                    break;
                }
            }
        }

        return indices;
    }

    SwapChainSupportInfo getSwapChainSupportInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        SwapChainSupportInfo info;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &info.surfaceCapabilities);

        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);

        if (surfaceFormatCount > 0) {
            info.formats.resize(surfaceFormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, info.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount > 0) {
            info.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, info.presentModes.data());
        }


        return info;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const glm::ivec2& framebufferSize) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D actualExtent = { static_cast<uint32_t>(framebufferSize.x), static_cast<uint32_t>(framebufferSize.y) };

            return actualExtent;
        }
    }

    void SwapChain::create(const glm::ivec2& framebufferSize) {
        SwapChainSupportInfo info = getSwapChainSupportInfo(physicalDevice, surface);

        auto surfaceFormat = chooseSwapSurfaceFormat(info.formats);
        auto presentMode = chooseSwapPresentMode(info.presentModes);
        extent = chooseSwapExtent(info.surfaceCapabilities, framebufferSize);

        // need to determine how many images to have int he swap chain.  Recommendation is to use one more than the minimum
        uint32_t imageCount = info.surfaceCapabilities.minImageCount + 1;

        if (info.surfaceCapabilities.maxImageCount > 0) { // 0 signifies no maximum
            imageCount = std::min(imageCount, info.surfaceCapabilities.maxImageCount);
        }

        // fill in the create info struct with the values from our choices above
        VkSwapchainCreateInfoKHR swapChainInfo = {};
        swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainInfo.surface = surface;
        swapChainInfo.minImageCount = imageCount;
        swapChainInfo.imageFormat = surfaceFormat.format;
        swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapChainInfo.imageExtent = extent;
        swapChainInfo.imageArrayLayers = 1;
        swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // signifies we will be rendering into the image

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

        uint32_t indiciesArray[] = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

        // if the queue families are the same then we can set up exclusive sharing mode for the swap chain.
        // This means that the queue for rendering and presentation are the same
        // If not we specificy concurrent queues
        if (indices.graphicsFamily == indices.presentationFamily) {
            swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainInfo.queueFamilyIndexCount = 0;
            swapChainInfo.pQueueFamilyIndices = nullptr;
        }
        else {
            swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainInfo.queueFamilyIndexCount = 2;
            swapChainInfo.pQueueFamilyIndices = indiciesArray;
        }

        swapChainInfo.preTransform = info.surfaceCapabilities.currentTransform; // allows for transforms such as rotate, etc...this sets default (no transform).
        swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        swapChainInfo.presentMode = presentMode;
        swapChainInfo.clipped = VK_TRUE; // optimiztion...but unable to read back pixels that are obscured by another window

        swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &handle) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain");
        }

        imageFormat = surfaceFormat.format;

        uint32_t swapChainImageCount = 0;
        vkGetSwapchainImagesKHR(device, handle, &swapChainImageCount, nullptr);
        images.resize(swapChainImageCount);
        vkGetSwapchainImagesKHR(device, handle, &swapChainImageCount, images.data());

        imageViews.resize(images.size());
        for (size_t i = 0; i < imageViews.size(); i++) {
            imageViews[i] = image::createView(device, images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void SwapChain::cleanup() {
        for (auto imageView : imageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, handle, nullptr); // this destroys the images in the chain
    }
}
