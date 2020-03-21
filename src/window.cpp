#include "vkdev/window.h"

#include <stdexcept>

namespace vkdev {

void Window::createWindow(int width, int height) {
    auto result = glfwInit();

    if (result == GLFW_FALSE) {
        throw std::runtime_error("failed it initialize GLFW.");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // Signal GLFW to not create openGL context
    glfwWindow = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

    if (!glfwWindow) {
        throw std::runtime_error("failed to create GLFW window");
    }

    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, onResize);
}

void Window::waitForMinimize() {
    // This code handles the case where glfw is processing a minimize event
    int width = 0, height = 0;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(glfwWindow, &width, &height);
        glfwWaitEvents();
    }
}

void Window::poll() {
    glfwPollEvents();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(glfwWindow) != 0;
}

bool Window::wasResized() const {
    return framebufferResized;
}

void Window::markResizeHandled() {
    framebufferResized = false;
}

void Window::onResize(GLFWwindow* glfwWindow_, int width, int height) {
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow_));
    window->framebufferResized = true;
}

void Window::createSurface() {

    if (glfwCreateWindowSurface(instance.handle, glfwWindow, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failure creating window surface");
    }
}

glm::ivec2 Window::getFramebufferSize() const {
    glm::ivec2 framebufferSize;
    glfwGetFramebufferSize(glfwWindow, &framebufferSize.x, &framebufferSize.y);

    return framebufferSize;
}

void Window::cleanupSurface() {
    vkDestroySurfaceKHR(instance.handle, surface, nullptr);
}

void Window::cleanupWindow() {
    if (glfwWindow) {
        glfwDestroyWindow(glfwWindow);
        glfwWindow = nullptr;
    }

    glfwTerminate();
}

}
