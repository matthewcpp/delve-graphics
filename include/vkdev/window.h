#pragma once

#include <vkdev/instance.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <memory>

namespace vkdev {

class Window {
public:
    explicit Window(Instance& instance_) : instance(instance_) {}

    // Window needs to be created before the vulkan instance.
    void createWindow(int width, int height);

    // Call this after the instance handle has been created.
    void createSurface();

    void cleanupSurface();
    void cleanupWindow();

    glm::ivec2 getFramebufferSize() const;

    void waitForMinimize();
    bool shouldClose() const;
    bool wasResized() const;
    void markResizeHandled();
    void poll();

    VkSurfaceKHR surface = VK_NULL_HANDLE;

private:
    static void onResize(GLFWwindow* glfwWindow_, int width, int height);
    
    Instance& instance;
    GLFWwindow* glfwWindow = nullptr;
    bool framebufferResized = false;
};

}
