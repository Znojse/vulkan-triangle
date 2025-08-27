#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vt::triangle {

class HelloTriangleApplication {
  public:
    HelloTriangleApplication()           = default;
    ~HelloTriangleApplication() noexcept = default;

    // Copy constructor and assignment operator.
    HelloTriangleApplication(const HelloTriangleApplication& other)            = delete;  // This is a comment.
    HelloTriangleApplication& operator=(const HelloTriangleApplication& other) = delete;

    // Move constructor and move assignment operator.
    HelloTriangleApplication(const HelloTriangleApplication&& other) noexcept            = delete;
    HelloTriangleApplication& operator=(const HelloTriangleApplication&& other) noexcept = delete;

    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

  private:
    GLFWwindow*               window;
    static constexpr uint32_t kWidth  = 800;
    static constexpr uint32_t kHeight = 600;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
};

}  // namespace vt::triangle
