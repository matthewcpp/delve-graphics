#pragma once

#include "instance.h"
#include "queue.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>

namespace vkdev {

class Device {
public:
    Device(Instance& instance_, VkSurfaceKHR surface_): instance(instance_), surface(surface_) {}

    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice logical = VK_NULL_HANDLE;

    void create(const std::vector<std::string>& requiredDeviceExtensions);
    VkSampleCountFlagBits getMaxSupportedSampleCount();

    Queue graphicsQueue;
    Queue presentationQueue;

private: 
    void createPhysicalDevice(const std::vector<std::string>& requiredDeviceExtensions);
    void createLogicalDevice(const std::vector<std::string>& requiredDeviceExtensions);

private:
    Instance& instance;
    VkSurfaceKHR surface;
};

}