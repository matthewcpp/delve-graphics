#include "vkdev/queue.h"

#include <vector>

namespace vkdev::queue {
    FamilyIndices findFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        FamilyIndices indices;

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
}