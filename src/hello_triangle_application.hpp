#pragma once

namespace vt::triangle {

class HelloTriangleApplication {
public:
    HelloTriangleApplication()  = default;
    ~HelloTriangleApplication() noexcept = default;

    // Copy constructor and assignment operator.
    HelloTriangleApplication(const HelloTriangleApplication& other)            = delete;
    HelloTriangleApplication& operator=(const HelloTriangleApplication& other) = delete;

    // Move constructor and move assignment operator.
    HelloTriangleApplication(const HelloTriangleApplication&& other)            noexcept = delete;
    HelloTriangleApplication& operator=(const HelloTriangleApplication&& other) noexcept = delete;

    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initVulkan();
    void mainLoop();
    void cleanup();
};

} // namespace vt::triangle
