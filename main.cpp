#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <algorithm>
#include <optional>
#include <set>
#include <limits>
#include <fstream>
#include <vector>
#include <array>

#include <string.h>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;
constexpr int MAX_SIMULTANEOUS_FRAMES = 2;

const std::vector<const char*> requiredValidationLayers = {
    "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Note that creation of the debug callback is optional.
// By default, if validation layers are enabled and no debug callback is set, Vulkan will output to standard out.
// Since all we are doing is outputting to standard out, this is technically unnecessary.
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE; // If true, call that generated callback should be aborted.  This should always be false.
}

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        // describes the format of the vertex
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;  // describes position in array of bindings
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // attribute descriptions...describe the attributes of the vertex, i.e. position and color,
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

        // describe position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0; //location in the shader ie. layout(LOCATION = 0) etc
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        //describe vertex color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = sizeof(glm::vec2);

        return attributeDescriptions;
    }
};

const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
};

// alignas is used to be explicit in regards to vulkan alignment requirements.  mat4 should be aligned to multiples of 16 bytes
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class VulkanTestApplication {
private:

    static void initMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
    }

    void createInstance() {
        // ApplicationInfo is optional but can allow for the driver to perhaps perform optimizations
        VkApplicationInfo appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Retrieve the needed Vulkan extensions for working with a GLFW window
        uint32_t glfwExtensionCount = 0;
        const char** glfwVulkanExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> requiredExtensions(glfwVulkanExtensions, glfwVulkanExtensions + glfwExtensionCount);

        // if logging is enabled we will create a special debug messenger to handle any message emitted during instance creation
        // Note this messenger will be cleaned up by the system.
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        auto supportedValidationLayers = getSupportedValidationLayers();
        if (_enableValidation) {
            if (supportedValidationLayers.empty()) {
                throw std::runtime_error("No supported validation layers found");
            }

            createInfo.enabledLayerCount = static_cast<uint32_t>(supportedValidationLayers.size());
            createInfo.ppEnabledLayerNames = supportedValidationLayers.data();

            // Add on the debug utilities extension
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            initMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vulkan instance");
        }
    }

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentationFamily; // allows for displaying to a surface

        inline bool isComplete() const { return graphicsFamily.has_value() && presentationFamily.has_value(); }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice) {
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
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, _surface, &presentationSupport);

            if (presentationSupport) {
                indices.presentationFamily = i;

                if (indices.isComplete()) {
                    break;
                }
            }
        }

        return indices;
    }

    struct SwapChainSupportInfo {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportInfo getSwapChainSupportInfo(VkPhysicalDevice physicalDevice) {
        SwapChainSupportInfo info;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &info.surfaceCapabilities);

        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &surfaceFormatCount, nullptr);

        if (surfaceFormatCount > 0) {
            info.formats.resize(surfaceFormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &surfaceFormatCount, info.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, nullptr);

        if (presentModeCount > 0) {
            info.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, info.presentModes.data());
        }


        return info;
    }

    bool deviceSupportsRequiredExtensions(VkPhysicalDevice physicalDevice) {
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

    bool phsicalDeviceIsSuitable(VkPhysicalDevice physicalDevice) {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        bool requiredExtensionsSupported = deviceSupportsRequiredExtensions(physicalDevice);

        if (requiredExtensionsSupported) {
            SwapChainSupportInfo info = getSwapChainSupportInfo(physicalDevice);
            bool swapChainAdequate = !info.formats.empty() && !info.presentModes.empty();

            return queueFamilyIndices.isComplete() && swapChainAdequate;
        }
        else {
            return false;
        }
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

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(_glfwWindow, &width, &height);
            VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

            return actualExtent;
        }
    }

    void createSwapChain() {
        SwapChainSupportInfo info = getSwapChainSupportInfo(_physicalDevice);

        auto surfaceFormat = chooseSwapSurfaceFormat(info.formats);
        auto presentMode = chooseSwapPresentMode(info.presentModes);
        auto extent = chooseSwapExtent(info.surfaceCapabilities);

        // need to determine how many images to have int he swap chain.  Reccomendation is to use one more than the minimum
        uint32_t imageCount = info.surfaceCapabilities.minImageCount + 1;

        if (info.surfaceCapabilities.maxImageCount > 0) { // 0 signifies no maximum
            imageCount = std::min(imageCount, info.surfaceCapabilities.maxImageCount);
        }

        // fill in the create info struct with the values from our choices above
        VkSwapchainCreateInfoKHR swapChainInfo = {};
        swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainInfo.surface = _surface;
        swapChainInfo.minImageCount = imageCount;
        swapChainInfo.imageFormat = surfaceFormat.format;
        swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapChainInfo.imageExtent = extent;
        swapChainInfo.imageArrayLayers = 1;
        swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // signifies we will be rendering into the image

        QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);

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

        if (vkCreateSwapchainKHR(_device, &swapChainInfo, nullptr, &_swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain");
        }

        uint32_t swapChainImageCount = 0;
        vkGetSwapchainImagesKHR(_device, _swapchain, &swapChainImageCount, nullptr);

        // resulting swap chain data is saved for ease future use.
        _swapChainImages.resize(swapChainImageCount);
        vkGetSwapchainImagesKHR(_device, _swapchain, &swapChainImageCount, _swapChainImages.data());

        _swapChainImageFormat = surfaceFormat.format;
        _swapChainExtent = extent;
    }

    void createImageViews() {
        _swapChainImageViews.resize(_swapChainImages.size());

        for (size_t i = 0; i < _swapChainImages.size(); i++) {
            VkImageViewCreateInfo imageViewInfo = {};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = _swapChainImages[i];
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = _swapChainImageFormat;

            // following settings allow us to make color chanels to one another.  This code sets the default values
            imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // subresource range describes purpose and access pattern
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(_device, &imageViewInfo, nullptr, &_swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views");
            }
        }
    }

    // By default we will query the available devices and pick the first suitable device
    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find a graphics card that supports vulkan");
        }

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDevices.data());

        for (auto device : physicalDevices) {
            if (phsicalDeviceIsSuitable(device)) {
                _physicalDevice = device;
                break;
            }
        }

        if (_physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice);

        // will need to create a device queue for each unique family.  It is possible that the diffent queue types will be part of the same family.
        std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos;
        std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentationFamily.value() };

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

        // For now we do not need any special features from our logical device.
        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        deviceCreateInfo.pQueueCreateInfos = deviceQueueInfos.data();
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueInfos.size());

        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        // we need to enable the swap chain extension so we can present to surfaces
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

        if (vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device.");
        }

        // after device creation is successful, need to grab a handle to our queues
        vkGetDeviceQueue(_device, queueFamilyIndices.graphicsFamily.value(), 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, queueFamilyIndices.presentationFamily.value(), 0, &_presentationQueue);
    }

    void createSurface() {
        if (glfwCreateWindowSurface(_instance, _glfwWindow, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failure creating window surface");
        }
    }

    void enableLogging() {
        if (!_enableValidation) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        initMessengerCreateInfo(createInfo);

        auto createDebugMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");

        if (!createDebugMessengerFunc) {
            throw std::runtime_error("unable to load vkCreateDebugUtilsMessengerEXT function");
        }

        if (createDebugMessengerFunc(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to create debug messenger");
        }
    }

    void terminateLogging() {
        if (!_enableValidation) return;

        auto destroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");

        if (destroyMessengerFunc) {
            destroyMessengerFunc(_instance, _debugMessenger, nullptr);
        }
        else {
            std::cout << "warning: unable to destroy debug logger." << std::endl;
        }
    }

    std::vector<const char*> getSupportedValidationLayers() {
        uint32_t validationLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

        std::vector<VkLayerProperties> availableLayerProperties(validationLayerCount);
        vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayerProperties.data());

        for (const auto& layer : availableLayerProperties) {
            std::cout << layer.layerName << std::endl;
        }

        std::vector<const char*> supportedValidationLayers;
        for (const auto requiredLayerName : requiredValidationLayers) {
            auto layerProperty = std::find_if(availableLayerProperties.begin(), availableLayerProperties.end(), [requiredLayerName](const VkLayerProperties& property) { return strcmp(property.layerName, requiredLayerName) == 0; });

            if (layerProperty != availableLayerProperties.end()) {
                supportedValidationLayers.push_back(requiredLayerName);
            }
        }

        return supportedValidationLayers;
    }

    std::vector<char> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("unable to read file: " + path);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo shaderInfo = {};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = code.size();
        shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;

        if (vkCreateShaderModule(_device, &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }

        return shaderModule;
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = _swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // multisampling

        // These apply to color and depth data
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // we will clear to black b4 rendering each frame
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // we arent doing anything with the stencil buffer
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // signals we will be presenting the image to surface in swap chain

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        // we need to configure the subpass of our renderpass to wait for the color attachment stage of the pipeline to begin
        // could also set waitStages for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        VkSubpassDependency subpassDependency = {};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;

        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask = 0;

        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &subpassDependency;

        if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass.");
        }
    }

    // search though all the available memory types to find the index of correct source
    // note that the memory type must be a match as well as the required properties (such as write access, etc)
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    // copying a vertex buffer requires a transfer command.  We will need to create a temporary command buffer to execute the command
    // Ideally it would be useful to create a separate command pool for short lived transfer operations like this as opposed to using the main command pool.
    // note that we are using the graphics queue to perform copies.  this is because graphics queues must also support buffer copy operations.
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
        commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocInfo.commandPool = _commandPool;
        commandBufferAllocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(_device, &commandBufferAllocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy regionToCopy = {}; // copy the whole buffer in one shot
        regionToCopy.srcOffset = 0;
        regionToCopy.dstOffset = 0;
        regionToCopy.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &regionToCopy);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(_graphicsQueue);

        vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = size; // buffer size in bytes
        vertexBufferInfo.usage = usage;
        vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(_device, &vertexBufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(_device, buffer, &memoryRequirements);

        // describe the memory we want to allocate into the buffer
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory.");
        }

        //now that memory is allocated, associate it with vertex buffer we just made above
        if (vkBindBufferMemory(_device, buffer, bufferMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("failed to bind memory to buffer");
        }
    }

    void createVertexBuffer() {
        // create a temporary CPU visible staging buffer to copy vertex data to the GPU (device local)
        const auto bufferSize = static_cast<VkDeviceSize>(sizeof(Vertex) * vertices.size());
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // copy the vertex data into the staging buffer
        void* stagingBufferData = nullptr;
        vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &stagingBufferData);
        memcpy(stagingBufferData, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(_device, stagingBufferMemory);

        // create our vertex buffer which will hold data on the GPU
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexBufferMemory);

        // copy the staging buffer to device local buffer
        copyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

        // cleanup staging buffer
        vkDestroyBuffer(_device, stagingBuffer, nullptr);
        vkFreeMemory(_device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        const auto bufferSize = static_cast<VkDeviceSize>(sizeof(uint16_t) * indices.size());
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* stagingBufferData = nullptr;
        vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &stagingBufferData);
        memcpy(stagingBufferData, indices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(_device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory);
        copyBuffer(stagingBuffer, _indexBuffer, bufferSize);

        vkDestroyBuffer(_device, stagingBuffer, nullptr);
        vkFreeMemory(_device, stagingBufferMemory, nullptr);
    }

    // a uniform buffer in our case is tired to a swap chain image
    // note that since the uniform values will be changing every frame (MVP matrix) we do not need a staging buffer or device memory location bit
    void createUniformBuffers(){
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        _uniformBuffers.resize(_swapChainImages.size());
        _uniformBuffersMemory.resize(_swapChainImages.size());

        for (size_t i = 0; i < _uniformBuffers.size(); i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffers[i], _uniformBuffersMemory[i]);
        }
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(_swapChainImages.size());

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(_swapChainImages.size());

        if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool.");
        }

    }

    // note that you do not need to explicitly clean up descriptor sets because they will be freed when their pool is destroyed.
    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(_swapChainImages.size(), _descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(_swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        _descriptorSets.resize(_swapChainImages.size());
        if (vkAllocateDescriptorSets(_device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets");
        }

        // descriptor sets have been allocated but now need to be configured
        for (size_t i = 0; i < _descriptorSets.size(); i++) {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = _uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = _descriptorSets[i];
            descriptorWrite.dstBinding = 0; // we gave our uniform buffer binding index 0 in the shader code
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo; // we are describing a buffer object so we are using this field
            descriptorWrite.pImageInfo = nullptr; // used for descriptors that refer to image data
            descriptorWrite.pTexelBufferView = nullptr; // used for descriptors that refer to buffer views

            vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    // We need to provide details about every descriptor binding used in the shaders for pipeline creation
    // note that the descriptor set remains valid even when creating new pipelines.
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0; // refers to binding 0 from the vertex shader
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1; // if an array, specifies the number of items in the array
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // specify which shaders are accessing the binding
        uboLayoutBinding.pImmutableSamplers = nullptr; // used for image sampling

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout");
        }
    }

    void createGraphicsPipeline() {
        auto vertexShaderCode = readFile("shaders/shader.vert.spv");
        auto fragmentShaderCode = readFile("shaders/shader.frag.spv");

        auto vertexShaderModule = createShaderModule(vertexShaderCode);
        auto fragmentShaderModule = createShaderModule(fragmentShaderCode);

        // shader stage describes which shader is our vertex / fragment shader
        VkPipelineShaderStageCreateInfo vertexStage = {};
        vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexStage.module = vertexShaderModule;
        vertexStage.pName = "main"; // this is the entrypoint for the shader.

        VkPipelineShaderStageCreateInfo fragmentStage = {};
        fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentStage.module = fragmentShaderModule;
        fragmentStage.pName = "main"; // this is the entrypoint for the shader.

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStage, fragmentStage };

        // get the data structures for binding the array and also describing its attributes.  See vertex struct.
        const auto bindingDescription = Vertex::getBindingDescription();
        const auto attributeDescriptions = Vertex::getAttributeDescriptions();

        // describe the input format of vertex data
        VkPipelineVertexInputStateCreateInfo vertexInput = {};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDescription;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

        // define the type of primitive we will be drawing
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_swapChainExtent.width);
        viewport.height = static_cast<float>(_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0,0 };
        scissor.extent = _swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f; // note setting this above 1.0 requires enabling of widelines GPU feature
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // note that this is changed to counter clockwise due to Y-flip in projection matrix

        // add constant value to depth or a bias
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // we will return to multisampling later
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1; // Optional
        pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr; // will come back to this later

        pipelineInfo.layout = _pipelineLayout;

        pipelineInfo.renderPass = _renderPass;
        pipelineInfo.subpass = 0;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline");
        }

        vkDestroyShaderModule(_device, vertexShaderModule, nullptr);
        vkDestroyShaderModule(_device, fragmentShaderModule, nullptr);
    }

    void createFramebuffers() {
        _swapChainFramebuffers.resize(_swapChainImageViews.size());

        for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
            VkImageView attachments[] = { _swapChainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = _renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = _swapChainExtent.width;
            framebufferInfo.height = _swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer for swap chain images");
            }
        }
    }

    void createCommandPool() {
        auto queueFamilyIndicies = findQueueFamilies(_physicalDevice);

        // command pool can only create commands on a particular type of queue.  In or case we are making graphics commands so needs to be associated with our graphics queue
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily.value();
        poolInfo.flags = 0;

        if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool");
        }
    }

    // drawing commands involves binding a framebuffer, we will have to record a command buffer for every image in the swap chain.
    // TODO: look into use of secondary command buffer
    void createCommandBuffers() {
        _commandBuffers.resize(_swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

        if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers");
        }

        for (size_t i = 0; i < _commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin command buffer recording");
            }

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = _renderPass;
            renderPassInfo.framebuffer = _swapChainFramebuffers[i];

            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = _swapChainExtent;

            VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

            VkBuffer vertexBuffers[] = { _vertexBuffer };
            VkDeviceSize offsets[] = { 0 };

            vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            // descriptor sets are not unqiue to graphics pipeline.  Therefore we need to specify we are binding to graphics (as opposed to compute)
            vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[i], 0, nullptr);

            vkCmdDrawIndexed(_commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(_commandBuffers[i]);

            if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer");
            }
        }
    }

    void createSyncObjects() {
        _imageAvailableSemaphores.resize(MAX_SIMULTANEOUS_FRAMES);
        _renderFinishedSemaphores.resize(MAX_SIMULTANEOUS_FRAMES);
        _inFlightFences.resize(MAX_SIMULTANEOUS_FRAMES);
        _inFLightImages.resize(_swapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_SIMULTANEOUS_FRAMES; i++) {
            if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(_device, &semaphoreInfo, NULL, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores");
            }
        }
    }

    void updateUniformBuffer(uint32_t bufferIndex) {
        // get the application time
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // define MVP
        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), _swapChainExtent.width / (float) _swapChainExtent.height, 0.1f, 10.0f);

        // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
        // The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
        // If you don't do this, then the image will be rendered upside down.
        ubo.proj[1][1] *= -1;

        // copy the MVP into the corresponding uniform buffer
        // TODO: look into using push constants for a more efficent means of passing the uniform data to shaders
        void* data = nullptr;
        vkMapMemory(_device, _uniformBuffersMemory[bufferIndex], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(_device, _uniformBuffersMemory[bufferIndex]);
    }

    void drawFrame() {
        vkWaitForFences(_device, 1, &_inFlightFences[currentFrameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());

        // get the next available image from the swap chain and signal the semaphore when its available
        // if there is an error we may need to recreate the swap chain.  I.E. Window is resized, etc.
        // we do not recreate swap chain in suboptimal state here because we have already acquired an image.  suboptimal return code is still considered a successful return value
        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(_device, _swapchain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image");
        }

        updateUniformBuffer(imageIndex);

        // Check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (_inFLightImages[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(_device, 1, &_inFLightImages[imageIndex], VK_TRUE, UINT64_MAX);
        }
        // Mark the image as now being in use by this frame
        _inFLightImages[imageIndex] = _inFlightFences[currentFrameIndex];

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // signals that the system needs to wait for _imageAvailableSemaphore to be signald before the color attachment stage can begin
        VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[currentFrameIndex] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // set the correct command buffer based on the swap chain image index.
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

        // set up the semaphore to be signaled when command is done
        VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[currentFrameIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(_device, 1, &_inFlightFences[currentFrameIndex]);

        if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[currentFrameIndex]) != VK_SUCCESS) {
            throw std::runtime_error("error submitting draw command");
        }

        // need to signal to swap chain that image is ready for presentation
        // it will wait for the render finished semaphore to be signaled before doing its thing
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { _swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        // returns same error codes as acquiring an image above.
        // in this case we will recreate the swap chain
        result = vkQueuePresentKHR(_presentationQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _glfwFramebufferResized) {
            _glfwFramebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image");
        }

        currentFrameIndex = (currentFrameIndex + 1) % MAX_SIMULTANEOUS_FRAMES;
    }


    static void glfwFramebufferResizeCallback(GLFWwindow* _glfwWindow, int width, int height) {
        auto app = reinterpret_cast<VulkanTestApplication*>(glfwGetWindowUserPointer(_glfwWindow));
        app->_glfwFramebufferResized = true;
    }

    void initWindow() {
        auto result = glfwInit();
        if (result == GLFW_FALSE) {
            throw std::runtime_error("failed it initialize GLFW.");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // Signal GLFW to not create openGL context
        _glfwWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        if (!_glfwWindow) {
            throw std::runtime_error("failed to create GLFW window");
        }

        glfwSetWindowUserPointer(_glfwWindow, this);
        glfwSetFramebufferSizeCallback(_glfwWindow, glfwFramebufferResizeCallback);
    }

    void initVulkan() {
        createInstance();
        enableLogging();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void recreateSwapChain() {
        // This code handles the case where glfw is processing a minimize event
        int width = 0, height = 0;
        glfwGetFramebufferSize(_glfwWindow, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(_glfwWindow, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(_device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(_glfwWindow)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(_device);
    }

    void cleanupSwapChain() {
        for (auto framebuffer : _swapChainFramebuffers) {
            vkDestroyFramebuffer(_device, framebuffer, nullptr);
        }

        vkFreeCommandBuffers(_device, _commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());

        vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        vkDestroyRenderPass(_device, _renderPass, nullptr);

        for (auto imageView : _swapChainImageViews) {
            vkDestroyImageView(_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        for (size_t i = 0; i < _uniformBuffers.size(); i++) {
            vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
            vkFreeMemory(_device, _uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
    }

    void cleanup() {
        cleanupSwapChain();

        vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);

        vkDestroyBuffer(_device, _indexBuffer, nullptr);
        vkFreeMemory(_device, _indexBufferMemory, nullptr);

        vkDestroyBuffer(_device, _vertexBuffer, nullptr);
        vkFreeMemory(_device, _vertexBufferMemory, nullptr);

        for (int i = 0; i < MAX_SIMULTANEOUS_FRAMES; i++) {
            vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(_device, _inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(_device, _commandPool, nullptr);

        vkDestroyDevice(_device, nullptr);
        terminateLogging();
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyInstance(_instance, nullptr);

        if (_glfwWindow) {
            glfwDestroyWindow(_glfwWindow);
        }

        glfwTerminate();
    }

public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

    inline void enableValidationLayers(bool enableValidation) { _enableValidation = enableValidation; }

private:
    GLFWwindow* _glfwWindow = nullptr;

    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _presentationQueue = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR _surface;

    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;

    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _graphicsPipeline;

    VkCommandPool _commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> _commandBuffers;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    std::vector<VkFence> _inFLightImages;
    size_t currentFrameIndex = 0;

    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer _indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _indexBufferMemory = VK_NULL_HANDLE;

    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;

    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> _descriptorSets;

    bool _glfwFramebufferResized = false;
    bool _enableValidation = false;
};

int main(int argc, char** argv) {
    VulkanTestApplication app;

#if NDEBUG
    app.enableValidationLayers(false);
#else
    app.enableValidationLayers(true);
#endif

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}