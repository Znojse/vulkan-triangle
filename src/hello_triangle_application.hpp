#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vt::triangle {

class HelloTriangleApplication {
  public:
    HelloTriangleApplication()           = default;
    ~HelloTriangleApplication() noexcept = default;

    // Copy constructor and assignment operator.
    HelloTriangleApplication(const HelloTriangleApplication& other)            = delete;
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
    const std::string         kClassName = "HelloTriangleApplication";
    static constexpr uint32_t kWidth     = 800;
    static constexpr uint32_t kHeight    = 600;

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };

#ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
#else
    static constexpr bool enableValidationLayers = true;
#endif

    VkInstance               instance       = nullptr;
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
    GLFWwindow*              window         = nullptr;

    void                                                initWindow();
    void                                                initVulkan();
    void                                                mainLoop();
    void                                                cleanup();
    void                                                createInstance();
    std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT> populateDebugMessengerCreateInfo();
    void                                                setupDebugMessenger();
    std::vector<const char*>                            getRequiredExtensions();
    void                                                checkValidationLayerSupport();
    void                                                checkExtensionSupport(const std::vector<const char*>& extension);
};

}  // namespace vt::triangle
