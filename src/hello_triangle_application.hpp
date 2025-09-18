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
    HelloTriangleApplication(const HelloTriangleApplication& other)                    = delete;
    auto operator=(const HelloTriangleApplication& other) -> HelloTriangleApplication& = delete;

    // Move constructor and move assignment operator.
    HelloTriangleApplication(const HelloTriangleApplication&& other) noexcept                    = delete;
    auto operator=(const HelloTriangleApplication&& other) noexcept -> HelloTriangleApplication& = delete;

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

        [[nodiscard]] auto isComplete() const -> bool { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    enum DeviceSuitabilityScore : uint16_t { LOW = 125, LOW_MEDIUM = 250, MEDIUM = 500, MEDIUM_HIGH = 750, HIGH = 1000 };

    const std::string         kClassName = "HelloTriangleApplication";
    static constexpr uint32_t kWidth     = 800;
    static constexpr uint32_t kHeight    = 600;

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
#else
    static constexpr bool enableValidationLayers = true;
#endif

    VkSemaphore imageAvailableSemaphore = {};
    VkSemaphore renderFinishedSemaphore = {};
    VkFence     inFlightFence           = {};

    VkInstance               instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    GLFWwindow*              window         = VK_NULL_HANDLE;
    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkDevice                 device         = VK_NULL_HANDLE;
    VkQueue                  graphicsQueue  = VK_NULL_HANDLE;
    VkQueue                  presentQueue   = VK_NULL_HANDLE;
    VkSurfaceKHR             surface        = VK_NULL_HANDLE;
    VkSwapchainKHR           swapChain      = VK_NULL_HANDLE;

    VkFormat         swapChainImageFormat = {};
    VkExtent2D       swapChainExtent      = {};
    VkRenderPass     renderPass           = {};
    VkPipelineLayout pipelineLayout       = {};
    VkPipeline       graphicsPipeline     = {};
    VkCommandPool    commandPool          = {};
    VkCommandBuffer  commandBuffer        = {};

    std::vector<VkImage>       swapChainImages;
    std::vector<VkImageView>   swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void drawFrame();
    void cleanup();

    void createInstance();
    auto getRequiredExtensions() -> std::vector<const char*>;

    void setupDebugMessenger();
    auto populateDebugMessengerCreateInfo() -> std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT>;

    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    auto rateDeviceSuitability(VkPhysicalDevice device) -> uint32_t;
    auto findQueueFamilies(VkPhysicalDevice device) -> QueueFamilyIndices;
    auto querySwapChainSupport(VkPhysicalDevice device) -> SwapChainSupportDetails;

    void        createSwapchain();
    void        createImageViews();
    static auto chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) -> VkSurfaceFormatKHR;
    static auto chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) -> VkPresentModeKHR;
    auto        chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D;

    void createRenderPass();
    void createGraphicsPipeline();
    auto createShaderModule(const std::vector<char>& code) -> VkShaderModule;

    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();
    void checkExtensionSupport(const std::vector<const char*>& extension);
    auto checkDeviceExtensionSupport(VkPhysicalDevice device) -> bool;
    void checkValidationLayerSupport();
};

}  // namespace vt::triangle
