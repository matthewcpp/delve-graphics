#include "vkdev/device.h"

#include "vkdev/queue.h"
#include "vkdev/swapchain.h"

#include <stdexcept>
#include <set>

namespace vkdev {

bool deviceSupportsRequiredExtensions(VkPhysicalDevice physicalDevice, const std::vector<std::string>& requiredDeviceExtensions) {
    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);

    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    for (const auto& deviceExtension : deviceExtensions) {
        requiredExtensions.erase(deviceExtension.extensionName);
    }

    return requiredExtensions.empty();
}

bool phsicalDeviceIsSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<std::string>& requiredDeviceExtensions) {
    // ensure that this physical device has both graphics and presentation queues.
    try {
        vkdev::Queue::findGraphicsQueueIndex(physicalDevice);
        vkdev::Queue::findPresentationQueueIndex(physicalDevice, surface);
    }
    catch (std::runtime_error e) {
        return false;
    }

    bool requiredExtensionsSupported = deviceSupportsRequiredExtensions(physicalDevice, requiredDeviceExtensions);

    if (requiredExtensionsSupported) {
        SwapChainSupportInfo info = SwapChainSupportInfo::getForDevice(physicalDevice, surface);
        bool swapChainAdequate = !info.formats.empty() && !info.presentModes.empty();

        // note that most modern hardware will support samplerAnisotropy but we will just confirm the same
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

        return swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }
    else {
        return false;
    }
}

void Device::createPhysicalDevice(const std::vector<std::string>& requiredDeviceExtensions) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance.handle, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find a graphics card that supports vulkan");
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance.handle, &deviceCount, physicalDevices.data());

    for (auto device : physicalDevices) {
        if (phsicalDeviceIsSuitable(device, surface, requiredDeviceExtensions)) {
            physical = device;
            return;
        }
    }

    throw std::runtime_error("failed to pick a suitable physical device.");
}

void Device::createLogicalDevice(const std::vector<std::string>& requiredDeviceExtensions) {
    graphicsQueue.index = vkdev::Queue::findGraphicsQueueIndex(physical);
    presentationQueue.index = vkdev::Queue::findPresentationQueueIndex(physical, surface);

    // will need to create a device queue for each unique family.  It is possible that the different queue types will be part of the same family.
    std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos;
    std::set<uint32_t> uniqueQueueFamilies = { graphicsQueue.index, presentationQueue.index };

    float queuePriority = 1.0f;
    for (const auto& queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo deviceQueueInfo = {};
        deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueInfo.queueFamilyIndex = queueFamily;
        deviceQueueInfo.queueCount = 1;

        // Priority must be specified even if only one queue
        deviceQueueInfo.pQueuePriorities = &queuePriority;

        deviceQueueInfos.push_back(deviceQueueInfo);
    }


    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // anisotropic filtering is disabled by default

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceCreateInfo.pQueueCreateInfos = deviceQueueInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueInfos.size());

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // we need to enable the swap chain extension so we can present to surfaces
    std::vector<const char*> extensionCstrVec;
    for (const auto& extensionStr : requiredDeviceExtensions) {
        extensionCstrVec.push_back(extensionStr.c_str());
    }
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionCstrVec.size());
    deviceCreateInfo.ppEnabledExtensionNames = extensionCstrVec.data();

    if (vkCreateDevice(physical, &deviceCreateInfo, nullptr, &logical) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device.");
    }

    // after device creation is successful, need to grab a handle to our queues
    vkGetDeviceQueue(logical, graphicsQueue.index, 0, &graphicsQueue.handle);
    vkGetDeviceQueue(logical, presentationQueue.index, 0, &presentationQueue.handle);
}

void Device::createAllocator(){
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physical;
    allocatorInfo.device = logical;
    allocatorInfo.instance = instance.handle;

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

void Device::create(const std::vector<std::string>& requiredDeviceExtensions) {
    createPhysicalDevice(requiredDeviceExtensions);
    createLogicalDevice(requiredDeviceExtensions);
    createAllocator();
}

void Device::cleanup() {
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(logical, nullptr);
}

// we need to find the max samples that are usable by both the color buff AND the depth buffer
VkSampleCountFlagBits Device::getMaxSupportedSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physical, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

}