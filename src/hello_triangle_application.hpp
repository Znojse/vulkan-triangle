#pragma once

#include <cstdlib>
#include <memory>
#include <optional>
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
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    const std::string         kClassName = "HelloTriangleApplication";
    static constexpr uint32_t kWidth     = 800;
    static constexpr uint32_t kHeight    = 600;

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
#else
    static constexpr bool enableValidationLayers = true;
#endif

    VkInstance                 instance              = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT   debugMessenger        = VK_NULL_HANDLE;
    GLFWwindow*                window                = VK_NULL_HANDLE;
    VkPhysicalDevice           physicalDevice        = VK_NULL_HANDLE;
    VkDevice                   device                = VK_NULL_HANDLE;
    VkQueue                    graphicsQueue         = VK_NULL_HANDLE;
    VkQueue                    presentQueue          = VK_NULL_HANDLE;
    VkSurfaceKHR               surface               = VK_NULL_HANDLE;
    VkSwapchainKHR             swapChain             = VK_NULL_HANDLE;
    VkFormat                   swapChainImageFormat  = {};
    VkExtent2D                 swapChainExtent       = {};
    std::vector<VkImage>       swapChainImages       = {};
    std::vector<VkImageView>   swapChainImageViews   = {};
    VkRenderPass               renderPass            = {};
    VkPipelineLayout           pipelineLayout        = {};
    VkPipeline                 graphicsPipeline      = {};
    std::vector<VkFramebuffer> swapChainFramebuffers = {};

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void                     createInstance();
    std::vector<const char*> getRequiredExtensions();

    void                                                setupDebugMessenger();
    std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT> populateDebugMessengerCreateInfo();

    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    int32_t                 rateDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    void               createSwapchain();
    void               createImageViews();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void           createRenderPass();
    void           createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void           createFramebuffers();

    void checkExtensionSupport(const std::vector<const char*>& extension);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void checkValidationLayerSupport();
};

}  // namespace vt::triangle
