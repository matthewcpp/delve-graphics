#include "vkdev/instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string.h>

const std::vector<const char*> requiredValidationLayers = {
    "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation"
};

namespace vkdev {
    std::vector<const char*> getSupportedValidationLayers() {
        uint32_t validationLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

        std::vector<VkLayerProperties> availableLayerProperties(validationLayerCount);
        vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayerProperties.data());

        //for (const auto& layer : availableLayerProperties) {
        //    std::cout << layer.layerName << std::endl;
        //}

        std::vector<const char*> supportedValidationLayers;
        for (const auto requiredLayerName : requiredValidationLayers) {
            auto layerProperty = std::find_if(availableLayerProperties.begin(), availableLayerProperties.end(), [requiredLayerName](const VkLayerProperties& property) { return strcmp(property.layerName, requiredLayerName) == 0; });

            if (layerProperty != availableLayerProperties.end()) {
                supportedValidationLayers.push_back(requiredLayerName);
            }
        }

        return supportedValidationLayers;
    }

    void Instance::create(bool enableValidationLayers) {
        // ApplicationInfo is optional but can allow for the driver to perhaps perform optimizations
        VkApplicationInfo appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "vulkantest";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "vkdev";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Retrieve the needed Vulkan extensions for working with a GLFW window
        uint32_t glfwExtensionCount = 0;
        const char** glfwVulkanExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> requiredExtensions(glfwVulkanExtensions, glfwVulkanExtensions + glfwExtensionCount);

        auto supportedValidationLayers = getSupportedValidationLayers();
        if (enableValidationLayers) {
            if (supportedValidationLayers.empty()) {
                throw std::runtime_error("No supported validation layers found");
            }

            createInfo.enabledLayerCount = static_cast<uint32_t>(supportedValidationLayers.size());
            createInfo.ppEnabledLayerNames = supportedValidationLayers.data();

            // Add on the debug utilities extension
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        if (vkCreateInstance(&createInfo, nullptr, &handle) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vulkan instance");
        }
    }

    void Instance::cleanup() {
        vkDestroyInstance(handle, nullptr);
    }
}