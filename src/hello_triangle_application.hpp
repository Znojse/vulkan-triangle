#pragma once

#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vt::triangle {

// NOLINTBEGIN(misc-include-cleaner)
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

    void Run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

  private:
    struct QueueFamilyIndices {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        [[nodiscard]] auto IsComplete() const -> bool { return graphicsFamily.has_value() && presentFamily.has_value(); }

        [[nodiscard]] auto GetGraphicsFamilyValue() -> uint32_t {
            if (!graphicsFamily.has_value()) {
                throw std::runtime_error(std::format("QueueFamilyIndices::getGraphicsFamilyValue: graphicsFamily is empty."));
            }

            return graphicsFamily.value();
        }

        [[nodiscard]] auto GetPresentFamilyValue() -> uint32_t {
            if (!presentFamily.has_value()) {
                throw std::runtime_error(std::format("QueueFamilyIndices::getPresentFamilyValue: presentFamily is empty."));
            }

            return presentFamily.value();
        }
    };

    struct SwapChainSupportDetails {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };

    enum DeviceSuitabilityScore : uint16_t { LOW = 125, LOW_MEDIUM = 250, MEDIUM = 500, MEDIUM_HIGH = 750, HIGH = 1000 };

    const std::string         kClassName = "HelloTriangleApplication";  // NOLINT(readability-identifier-naming)
    static constexpr uint32_t kWidth     = 800;
    static constexpr uint32_t kHeight    = 600;

    const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
    static constexpr bool kEnableValidationLayers = false;
#else
    static constexpr bool kEnableValidationLayers = true;
#endif

    VkSemaphore m_imageAvailableSemaphore = {};
    VkSemaphore m_renderFinishedSemaphore = {};
    VkFence     m_inFlightFence           = {};

    VkInstance               m_instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    GLFWwindow*              m_window         = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                 m_device         = VK_NULL_HANDLE;
    VkQueue                  m_graphicsQueue  = VK_NULL_HANDLE;
    VkQueue                  m_presentQueue   = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface        = VK_NULL_HANDLE;
    VkSwapchainKHR           m_swapChain      = VK_NULL_HANDLE;

    std::vector<VkImage>       m_swapChainImages;
    std::vector<VkImageView>   m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    VkFormat         m_swapChainImageFormat = {};
    VkExtent2D       m_swapChainExtent      = {};
    VkRenderPass     m_renderPass           = {};
    VkPipelineLayout m_pipelineLayout       = {};
    VkPipeline       m_graphicsPipeline     = {};
    VkCommandPool    m_commandPool          = {};
    VkCommandBuffer  m_commandBuffer        = {};

    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void DrawFrame();
    void Cleanup();

    void CreateInstance();
    auto GetRequiredExtensions() -> std::vector<const char*>;

    void SetupDebugMessenger();
    auto PopulateDebugMessengerCreateInfo() -> std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT>;

    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    auto RateDeviceSuitability(VkPhysicalDevice device) -> uint32_t;
    auto FindQueueFamilies(VkPhysicalDevice device) -> QueueFamilyIndices;
    auto QuerySwapChainSupport(VkPhysicalDevice device) -> SwapChainSupportDetails;

    void CreateSwapchain();
    void CreateImageViews();
    auto ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D;

    void CreateRenderPass();
    void CreateGraphicsPipeline();
    auto CreateShaderModule(const std::vector<char>& code) -> VkShaderModule;

    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void CreateSyncObjects();
    void CheckExtensionSupport(const std::vector<const char*>& extension);
    auto CheckDeviceExtensionSupport(VkPhysicalDevice device) -> bool;
    void CheckValidationLayerSupport();
};
// NOLINTEND(misc-include-cleaner)

}  // namespace vt::triangle
