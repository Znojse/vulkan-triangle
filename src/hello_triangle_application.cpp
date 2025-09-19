#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "hello_triangle_application.hpp"
#include "utilities.hpp"
#include "vulkan_validation.hpp"

namespace vt::triangle {

// NOLINTBEGIN(misc-include-cleaner)
void HelloTriangleApplication::InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void HelloTriangleApplication::MainLoop() {
    while (0 == glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(m_device);
}

void HelloTriangleApplication::DrawFrame() {
    // Common steps:
    //  - Wait for the previous frame to finish
    //  - Acquire an image from the swap chain
    //  - Record a command buffer which draws the scene onto that image
    //  - Submit the recorded command buffer
    //  - Present the swap chain image
    vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFence);

    uint32_t imageIndex = { 0 };
    vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    vkResetCommandBuffer(m_commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
    RecordCommandBuffer(m_commandBuffer, imageIndex);

    // Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores.
    std::array<VkSemaphore, 1>          waitSemaphores   = { m_imageAvailableSemaphore };
    std::array<VkSemaphore, 1>          signalSemaphores = { m_renderFinishedSemaphore };
    std::array<VkPipelineStageFlags, 1> waitStages       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSubmitInfo                  submitInfo       = { .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                                             .pNext                = nullptr,
                                                             .waitSemaphoreCount   = 1,
                                                             .pWaitSemaphores      = waitSemaphores.data(),
                                                             .pWaitDstStageMask    = waitStages.data(),
                                                             .commandBufferCount   = 1,
                                                             .pCommandBuffers      = &m_commandBuffer,
                                                             .signalSemaphoreCount = 1,
                                                             .pSignalSemaphores    = signalSemaphores.data() };

    if (const auto& result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::DrawFrame: Failed to submit draw command buffer, error code: {}.", kClassName, result));
    }

    std::array<VkSwapchainKHR, 1> swapChains  = { m_swapChain };
    const VkPresentInfoKHR        presentInfo = {
               .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
               .pNext              = nullptr,
               .waitSemaphoreCount = 1,
               .pWaitSemaphores    = signalSemaphores.data(),
               .swapchainCount     = 1,
               .pSwapchains        = swapChains.data(),
               .pImageIndices      = &imageIndex,
               .pResults           = nullptr  // Optional
    };

    if (const auto& result = vkQueuePresentKHR(m_presentQueue, &presentInfo) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::DrawFrame: Failed to present image, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::Cleanup() {
    vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
    vkDestroyFence(m_device, m_inFlightFence, nullptr);

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (auto* framebuffer : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (auto* imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    vkDestroyDevice(m_device, nullptr);

    if (kEnableValidationLayers) {
        validation::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void HelloTriangleApplication::CreateInstance() {
    // AppInfo initializaton.
    const VkApplicationInfo appInfo = { .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                        .pNext              = nullptr,
                                        .pApplicationName   = "Hello Triangle",
                                        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                        .pEngineName        = "No Engine",
                                        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
                                        .apiVersion         = VK_API_VERSION_1_0 };

    // CreateInfo initialization and extensions.
    const auto& extensions = GetRequiredExtensions();
    CheckExtensionSupport(extensions);

    VkInstanceCreateInfo createInfo = { .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                        .pNext                   = nullptr,
                                        .flags                   = {},
                                        .pApplicationInfo        = &appInfo,
                                        .enabledLayerCount       = 0,
                                        .ppEnabledLayerNames     = nullptr,
                                        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
                                        .ppEnabledExtensionNames = extensions.data() };

    // Validation layers and debug if enabled.
    std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT> debugCreateInfo = { nullptr };
    if (kEnableValidationLayers) {
        CheckValidationLayerSupport();

        createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();

        debugCreateInfo  = PopulateDebugMessengerCreateInfo();
        createInfo.pNext = debugCreateInfo.get();
    }

    if (const auto& result = vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateInstance: Failed to create instance, error code: {}.", kClassName, result));
    }
}

auto HelloTriangleApplication::GetRequiredExtensions() -> std::vector<const char*> {
    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (nullptr == glfwExtensions) {
        throw std::runtime_error(std::format("{}::GetRequiredExtensions: Failed to get GLFW required instance extensions.", kClassName));
    }

    const std::span<const char*> glfwExtensionsSpan = { glfwExtensions, glfwExtensionCount };
    std::vector<const char*>     extensions         = {};
    extensions.reserve(glfwExtensionsSpan.size());

    for (const auto& extension : glfwExtensionsSpan) {
        extensions.emplace_back(extension);
    }

    if (kEnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void HelloTriangleApplication::SetupDebugMessenger() {
    if (!kEnableValidationLayers) {
        return;
    }

    auto createInfo = PopulateDebugMessengerCreateInfo();
    if (const auto& result = validation::vkCreateDebugUtilsMessengerEXT(m_instance, createInfo.get(), nullptr, &m_debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::SetupDebugMessenger: Failed to set up debug messenger, error code: {}.", kClassName, result));
    }
}

auto HelloTriangleApplication::PopulateDebugMessengerCreateInfo() -> std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT> {
    auto createInfo             = std::make_shared<VkDebugUtilsMessengerCreateInfoEXT>();
    createInfo->sType           = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    createInfo->messageSeverity = static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) |
                                  static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) |
                                  static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);

    createInfo->messageType = static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) |
                              static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) |
                              static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);

    createInfo->pfnUserCallback = validation::DebugCallback;
    createInfo->pUserData       = this;  // Optional - nullptr

    return createInfo;
}

void HelloTriangleApplication::CreateSurface() {
    if (const auto& result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateSurface: Failed to create window surface, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (0 == deviceCount) {
        throw std::runtime_error(std::format("{}::PickPhysicalDevice: Failed to find GPUs with Vulkan support!", kClassName));
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score.
    std::multimap<int32_t, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        const uint32_t score = RateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        m_physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error(std::format("{}::PickPhysicalDevice: Failed to find a suitable GPU!", kClassName));
    }

    for (auto candidate : candidates) {
        std::cout << std::format("{}::PickPhysicalDevice: Device score = {}.\n", kClassName, candidate.first);
    }
}

void HelloTriangleApplication::CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos    = {};
    const std::set<uint32_t>             uniqueQueueFamilies = { indices.GetGraphicsFamilyValue(), indices.GetPresentFamilyValue() };

    const float queuePriority = 1.0F;
    for (const uint32_t queueFamily : uniqueQueueFamilies) {
        // clang-format off
        const VkDeviceQueueCreateInfo queueCreateInfo = { .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                    .pNext            = nullptr,
                                                    .flags            = {},
                                                    .queueFamilyIndex = queueFamily,
                                                    .queueCount       = 1,
                                                    .pQueuePriorities = &queuePriority };
        // clang-format on
        queueCreateInfos.push_back(queueCreateInfo);
    }

    const VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = { .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                      .pNext                   = nullptr,
                                      .flags                   = {},
                                      .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
                                      .pQueueCreateInfos       = queueCreateInfos.data(),
                                      .enabledLayerCount       = 0,        // Deprecated.
                                      .ppEnabledLayerNames     = nullptr,  // Deprecated.
                                      .enabledExtensionCount   = static_cast<uint32_t>(m_deviceExtensions.size()),
                                      .ppEnabledExtensionNames = m_deviceExtensions.data(),
                                      .pEnabledFeatures        = &deviceFeatures };

    if (kEnableValidationLayers) {
        // Deprecated, but can be good to set anyways to be compatible with older implementations.
        createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }

    if (const auto& result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateLogicalDevice: Failed to logical device, error code: {}.", kClassName, result));
    }

    // Retrieve the queue handles for each QueueFamily.
    vkGetDeviceQueue(m_device, indices.GetGraphicsFamilyValue(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.GetPresentFamilyValue(), 0, &m_presentQueue);
}

auto HelloTriangleApplication::RateDeviceSuitability(VkPhysicalDevice device) -> uint32_t {
    uint32_t score = 0;

    const QueueFamilyIndices      indices             = FindQueueFamilies(device);
    const bool                    extensionSupported  = CheckDeviceExtensionSupport(device);
    const SwapChainSupportDetails swapChainSupport    = QuerySwapChainSupport(device);
    const bool                    isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    // Device is not supported, return a score of 0.
    if (!indices.IsComplete() || !extensionSupported || !isSwapChainAdequate) {
        return 0;
    }

    // Same queue family, boost the score.
    if (indices.graphicsFamily == indices.presentFamily) {
        score += DeviceSuitabilityScore::MEDIUM;
    }

    // Properties and features.
    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceFeatures   deviceFeatures   = {};
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Discrete GPUs have a significant performance advantage.
    if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == deviceProperties.deviceType) {
        score += DeviceSuitabilityScore::HIGH;
    }

    // Maximum possible size of textures affects graphics quality.
    score += deviceProperties.limits.maxImageDimension2D;

    // Example
    // // Application can't function without geometry shaders
    // if (!deviceFeatures.geometryShader) {
    //     return 0;
    // }

    return score;
}

auto HelloTriangleApplication::FindQueueFamilies(VkPhysicalDevice device) -> HelloTriangleApplication::QueueFamilyIndices {
    QueueFamilyIndices indices          = {};
    uint32_t           queueFamilyCount = { 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t idx = 0;
    for (const auto& qFamily : queueFamilies) {
        auto presentSupport = static_cast<VkBool32>(false);
        vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, m_surface, &presentSupport);

        if (static_cast<bool>(presentSupport)) {
            indices.presentFamily = idx;
        }

        if (0 != (qFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.graphicsFamily = idx;
        }

        // Return if a queue family is found.
        if (indices.IsComplete()) {
            return indices;
        }

        idx++;
    }

    return indices;
}

auto HelloTriangleApplication::QuerySwapChainSupport(VkPhysicalDevice device) -> HelloTriangleApplication::SwapChainSupportDetails {
    SwapChainSupportDetails details = {};

    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    // Surface formats (pixel format, color space)
    uint32_t formatCount = { 0 };
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (0 != formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    // Available presentation modes
    uint32_t presentModeCount = { 0 };
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (0 != presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void HelloTriangleApplication::CreateSwapchain() {
    const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice);
    const VkSurfaceFormatKHR      surfaceFormat    = utilities::ChooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR        presentMode      = utilities::ChooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D              extent           = ChooseSwapExtent(swapChainSupport.capabilities);

    // Decide how many images we would like to have in the swap chain.
    // The implementation specifies the minimum number that it requires to function.
    // However, simply sticking to this minimum means that we may sometimes have to wait on the driver
    // to complete internal operations before we can acquire another image to render to.
    // Therefore it is recommended to request at least one more image than the minimum.
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // Where 'maxImageCount' == 0 is a special value that means that there is no maximum.
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    QueueFamilyIndices       indices            = FindQueueFamilies(m_physicalDevice);
    std::array<uint32_t, 2>  queueFamilyIndices = { indices.GetGraphicsFamilyValue(), indices.GetPresentFamilyValue() };
    VkSwapchainCreateInfoKHR createInfo         = { .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                                    .pNext                 = nullptr,
                                                    .flags                 = {},
                                                    .surface               = m_surface,
                                                    .minImageCount         = imageCount,
                                                    .imageFormat           = surfaceFormat.format,
                                                    .imageColorSpace       = surfaceFormat.colorSpace,
                                                    .imageExtent           = extent,
                                                    .imageArrayLayers      = 1,
                                                    .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                    .imageSharingMode      = VK_SHARING_MODE_CONCURRENT,
                                                    .queueFamilyIndexCount = 2,
                                                    .pQueueFamilyIndices   = queueFamilyIndices.data(),
                                                    .preTransform          = swapChainSupport.capabilities.currentTransform,
                                                    .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                    .presentMode           = presentMode,
                                                    .clipped               = VK_TRUE,
                                                    .oldSwapchain          = VK_NULL_HANDLE };

    // Update to exclusive mode if the image can be owned by one queue family.
    if (indices.graphicsFamily == indices.presentFamily) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
    }

    if (const auto& result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateSwapchain: Failed to create swap chain, error code: {}.", kClassName, result));
    }

    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent      = extent;
}

void HelloTriangleApplication::CreateImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());
    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        const VkImageViewCreateInfo createInfo { .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                                 .pNext            = nullptr,
                                                 .flags            = {},
                                                 .image            = m_swapChainImages[i],
                                                 .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                                                 .format           = m_swapChainImageFormat,
                                                 .components       = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                                       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                                       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                                       .a = VK_COMPONENT_SWIZZLE_IDENTITY },
                                                 .subresourceRange = {
                                                     .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                     .baseMipLevel   = 0,
                                                     .levelCount     = 1,
                                                     .baseArrayLayer = 0,
                                                     .layerCount     = 1,
                                                 } };

        if (const auto& result = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error(std::format("{}::CreateImageViews: Failed to create image views, error code: {}.", kClassName, result));
        }
    }
}

auto HelloTriangleApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int32_t width  = { 0 };
    int32_t height = { 0 };
    glfwGetFramebufferSize(m_window, &width, &height);

    VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width      = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height     = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void HelloTriangleApplication::CreateRenderPass() {
    const VkAttachmentDescription colorAttachment { .flags          = {},
                                                    .format         = m_swapChainImageFormat,
                                                    .samples        = VK_SAMPLE_COUNT_1_BIT,
                                                    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                                    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                                                    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

    const VkAttachmentReference colorAttachmentRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    const VkSubpassDescription subpass = { .flags                   = {},
                                           .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                           .inputAttachmentCount    = 0,
                                           .pInputAttachments       = nullptr,
                                           .colorAttachmentCount    = 1,
                                           .pColorAttachments       = &colorAttachmentRef,
                                           .pResolveAttachments     = nullptr,
                                           .pDepthStencilAttachment = nullptr,
                                           .preserveAttachmentCount = 0,
                                           .pPreserveAttachments    = nullptr };

    const VkSubpassDependency dependency = { .srcSubpass      = VK_SUBPASS_EXTERNAL,
                                             .dstSubpass      = 0,
                                             .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                             .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                             .srcAccessMask   = 0,
                                             .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                             .dependencyFlags = {} };

    const VkRenderPassCreateInfo renderPassInfo = { .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                                    .pNext           = nullptr,
                                                    .flags           = {},
                                                    .attachmentCount = 1,
                                                    .pAttachments    = &colorAttachment,
                                                    .subpassCount    = 1,
                                                    .pSubpasses      = &subpass,
                                                    .dependencyCount = 1,
                                                    .pDependencies   = &dependency };

    if (const auto& result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateRenderPass: Failed to create render pass, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::CreateGraphicsPipeline() {
    const std::filesystem::path shaderPath     = std::filesystem::current_path() += std::filesystem::path("/build/vulkan-triangle/src");
    const auto                  vertShaderCode = utilities::ReadBinaryFile(std::filesystem::path(shaderPath / "triangle.vert.spv").string());
    const auto                  fragShaderCode = utilities::ReadBinaryFile(std::filesystem::path(shaderPath / "triangle.frag.spv").string());

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    const VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = {},
        .stage               = VK_SHADER_STAGE_VERTEX_BIT,
        .module              = vertShaderModule,
        .pName               = "main",  // Entrypoint - can combine e.g. multiple modules.
        .pSpecializationInfo = nullptr  // Optional   - specify values for shader constants.
    };

    const VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = {},
        .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module              = fragShaderModule,
        .pName               = "main",  // Entrypoint - can combine e.g. multiple modules.
        .pSpecializationInfo = nullptr  // Optional   - specify values for shader constants.
    };

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages  = { vertShaderStageInfo, fragShaderStageInfo };
    std::vector<VkDynamicState>                    dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    // clang-format off
    const VkPipelineDynamicStateCreateInfo dynamicState  = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = {},
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data()
    };

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = {},
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = nullptr
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // VkViewport viewport {
    //     .x = 0.0F,
    //     .y = 0.0F,
    //     .width = static_cast<float>(m_swapChainExtent.width),
    //     .height = static_cast<float>(m_swapChainExtent.height),
    //     .minDepth = 0.0F,
    //     .maxDepth = 1.0F
    // };

    // VkRect2D scissor = {
    //     .offset = {0, 0},
    //     .extent = m_swapChainExtent
    // };

    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = {},
        .viewportCount = 1,
        .pViewports    = nullptr,  // Dynamic, is set later.
        // .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = nullptr   // Dynamic, is set later.
        // .pScissors     = &scissor
    };

    const VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = {},
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0F,  // Optional
        .depthBiasClamp          = 0.0F,  // Optional
        .depthBiasSlopeFactor    = 0.0F,  // Optional
        .lineWidth               = 1.0F
    };

    const VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = {},
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0F,      // Optional
        .pSampleMask           = nullptr,   // Optional
        .alphaToCoverageEnable = VK_FALSE,  // Optional
        .alphaToOneEnable      = VK_FALSE   // Optional
    };

    // Required if you are using a depth and/or stencil buffer.
    // But we don't have one, so can pass nullptr later.
    // VkPipelineDepthStencilStateCreateInfo:

    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        // .blendEnable         = VK_TRUE;
        // .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        // .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        // .colorBlendOp        = VK_BLEND_OP_ADD;
        // .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        // .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        // .alphaBlendOp        = VK_BLEND_OP_ADD;
        .blendEnable         = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,   // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,  // Optional
        .colorBlendOp        = VK_BLEND_OP_ADD,       // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,   // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,  // Optional
        .alphaBlendOp        = VK_BLEND_OP_ADD,       // Optional
        .colorWriteMask      = static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_R_BIT) |
                               static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_G_BIT) |
                               static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_B_BIT) |
                               static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_A_BIT)
    };

    const VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = {},
        .logicOpEnable     = VK_FALSE,
        .logicOp           = VK_LOGIC_OP_COPY,           // Optional
        .attachmentCount   = 1,
        .pAttachments      = &colorBlendAttachment,
        .blendConstants    = { 0.0F, 0.0F, 0.0F, 0.0F }  // Optional
    };

    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .setLayoutCount         = 0,        // Optional
        .pSetLayouts            = nullptr,  // Optional
        .pushConstantRangeCount = 0,        // Optional
        .pPushConstantRanges    = nullptr   // Optional
    };
    // clang-format on

    if (const auto& result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateGraphicsPipeline: Failed to create pipeline layout, error code: {}.", kClassName, result));
    }

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = {},
        .stageCount          = 2,
        .pStages             = shaderStages.data(),
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState  = nullptr,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = nullptr,  // Optional
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = m_pipelineLayout,
        .renderPass          = m_renderPass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,  // Optional
        .basePipelineIndex   = -1               // Optional
    };

    if (const auto& result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateGraphicsPipeline: Failed to create graphic pipeline, error code: {}.", kClassName, result));
    }

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

auto HelloTriangleApplication::CreateShaderModule(const std::vector<char>& code) -> VkShaderModule {
    // clang-format off
    const VkShaderModuleCreateInfo createInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = {},
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<const uint32_t*>(code.data())
    };
    // clang-format on

    VkShaderModule shaderModule {};
    if (const auto& result = vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateShaderModule: Failed to create shader module, error code: {}.", kClassName, result));
    }

    return shaderModule;
}

void HelloTriangleApplication::CreateFramebuffers() {
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        std::array<VkImageView, 1>    attachments     = { m_swapChainImageViews[i] };
        const VkFramebufferCreateInfo framebufferInfo = { .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                          .pNext           = nullptr,
                                                          .flags           = {},
                                                          .renderPass      = m_renderPass,
                                                          .attachmentCount = 1,
                                                          .pAttachments    = attachments.data(),
                                                          .width           = m_swapChainExtent.width,
                                                          .height          = m_swapChainExtent.height,
                                                          .layers          = 1 };

        if (const auto& result = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error(std::format("{}::CreateFramebuffers: Failed to create framebuffer, error code: {}.", kClassName, result));
        }
    }
}

void HelloTriangleApplication::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

    const VkCommandPoolCreateInfo poolInfo = { .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                               .pNext            = nullptr,
                                               .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                               .queueFamilyIndex = queueFamilyIndices.GetGraphicsFamilyValue() };

    if (const auto& result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateCommandPool: Failed to create command pool, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::CreateCommandBuffers() {
    // clang-format off
    const VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = m_commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    // clang-format on

    if (const auto& result = vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::CreateCommandBuffers: Failed to create command buffer, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    const VkCommandBufferBeginInfo beginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = {},      // Optional
        .pInheritanceInfo = nullptr  // Optional
    };

    if (const auto& result = vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::RecordCommandBuffer: Failed to begin recording command buffer, error code: {}.", kClassName, result));
    }

    // clang-format off
    const VkClearValue          clearColor     = { { { 0.0F, 0.0F, 0.0F, 1.0F } } };
    const VkRenderPassBeginInfo renderPassInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = nullptr,
        .renderPass      = m_renderPass,
        .framebuffer     = m_swapChainFramebuffers[imageIndex],
        .renderArea {
            .offset      = { 0, 0 },
            .extent      = m_swapChainExtent
        },
        .clearValueCount = 1,
        .pClearValues    = &clearColor };
    // clang-format on

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    // clang-format off
    const VkViewport viewport {
        .x = 0.0F,
        .y = 0.0F,
        .width = static_cast<float>(m_swapChainExtent.width),
        .height = static_cast<float>(m_swapChainExtent.height),
        .minDepth = 0.0F,
        .maxDepth = 1.0F
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = m_swapChainExtent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    // clang-format on

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (const auto& result = vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::RecordCommandBuffer: Failed to end command buffer, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::CreateSyncObjects() {
    // clang-format off
    const VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = {} };
    const VkFenceCreateInfo     fenceInfo     = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,     .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

    if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(    m_device, &fenceInfo,     nullptr, &m_inFlightFence)           != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::RecordCommandBuffer: Failed to create semaphores/fence!", kClassName));
    }
    // clang-format on
}

void HelloTriangleApplication::CheckExtensionSupport(const std::vector<const char*>& extensions) {
    uint32_t availableCount { 0 };
    vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(availableCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, availableExtensions.data());

    for (std::string extensionName : extensions) {
        bool found { false };

        for (auto& availableExtension : availableExtensions) {
            if (extensionName == static_cast<const char*>(availableExtension.extensionName)) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error(std::format("{}::CheckExtensionSupport: Missing required extension: [{}].", kClassName, extensionName));
        }
    }
}

auto HelloTriangleApplication::CheckDeviceExtensionSupport(VkPhysicalDevice device) -> bool {
    uint32_t extensionCount = { 0 };
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(static_cast<const char*>(extension.extensionName));
    }

    return requiredExtensions.empty();
}

void HelloTriangleApplication::CheckValidationLayerSupport() {
    uint32_t availableCount { 0 };
    vkEnumerateInstanceLayerProperties(&availableCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(availableCount);
    vkEnumerateInstanceLayerProperties(&availableCount, availableLayers.data());

    for (const std::string layerName : m_validationLayers) {
        bool found { false };

        for (auto& availableLayer : availableLayers) {
            if (layerName == static_cast<const char*>(availableLayer.layerName)) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error(std::format("{}::CheckValidationLayerSupport: Missing required layer [{}].", kClassName, layerName));
        }
    }
}
// NOLINTEND(misc-include-cleaner)

}  // namespace vt::triangle
