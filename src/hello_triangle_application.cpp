#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "hello_triangle_application.hpp"
#include "utilities.hpp"
#include "vulkan_validation.hpp"

namespace vt::triangle {

void HelloTriangleApplication::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApplication::mainLoop() {
    while (0 == glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void HelloTriangleApplication::drawFrame() {
    // Common steps:
    //  - Wait for the previous frame to finish
    //  - Acquire an image from the swap chain
    //  - Record a command buffer which draws the scene onto that image
    //  - Submit the recorded command buffer
    //  - Present the swap chain image
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex = { 0 };
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(commandBuffer, imageIndex);

    // Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores.
    std::array<VkSemaphore, 1>          waitSemaphores   = { imageAvailableSemaphore };
    std::array<VkSemaphore, 1>          signalSemaphores = { renderFinishedSemaphore };
    std::array<VkPipelineStageFlags, 1> waitStages       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo                        submitInfo       = { .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                                             .pNext                = nullptr,
                                                             .waitSemaphoreCount   = 1,
                                                             .pWaitSemaphores      = waitSemaphores.data(),
                                                             .pWaitDstStageMask    = waitStages.data(),
                                                             .commandBufferCount   = 1,
                                                             .pCommandBuffers      = &commandBuffer,
                                                             .signalSemaphoreCount = 1,
                                                             .pSignalSemaphores    = signalSemaphores.data() };

    if (const auto& result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::drawFrame: Failed to submit draw command buffer, error code: {}.", kClassName, result));
    }

    std::array<VkSwapchainKHR, 1> swapChains  = { swapChain };
    VkPresentInfoKHR              presentInfo = {
                     .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                     .pNext              = nullptr,
                     .waitSemaphoreCount = 1,
                     .pWaitSemaphores    = signalSemaphores.data(),
                     .swapchainCount     = 1,
                     .pSwapchains        = swapChains.data(),
                     .pImageIndices      = &imageIndex,
                     .pResults           = nullptr  // Optional
    };

    if (const auto& result = vkQueuePresentKHR(presentQueue, &presentInfo) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::drawFrame: Failed to present image, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::cleanup() {
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    for (auto* framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto* imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        validation::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void HelloTriangleApplication::createInstance() {
    // AppInfo initializaton.
    VkApplicationInfo appInfo = { .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                  .pNext              = nullptr,
                                  .pApplicationName   = "Hello Triangle",
                                  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                  .pEngineName        = "No Engine",
                                  .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
                                  .apiVersion         = VK_API_VERSION_1_0 };

    // CreateInfo initialization and extensions.
    const auto& extensions = getRequiredExtensions();
    checkExtensionSupport(extensions);

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
    if (enableValidationLayers) {
        checkValidationLayerSupport();

        createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        debugCreateInfo  = populateDebugMessengerCreateInfo();
        createInfo.pNext = debugCreateInfo.get();
    }

    if (const auto& result = vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createInstance: Failed to create instance, error code: {}.", kClassName, result));
    }
}

auto HelloTriangleApplication::getRequiredExtensions() -> std::vector<const char*> {
    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (nullptr == glfwExtensions) {
        throw std::runtime_error(std::format("{}::getRequiredExtensions: Failed to get GLFW required instance extensions.", kClassName));
    }

    std::span<const char*>   glfwExtensionsSpan = { glfwExtensions, glfwExtensionCount };
    std::vector<const char*> extensions         = {};
    extensions.reserve(glfwExtensionsSpan.size());

    for (const auto& extension : glfwExtensionsSpan) {
        extensions.emplace_back(extension);
    }

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void HelloTriangleApplication::setupDebugMessenger() {
    if (!enableValidationLayers) {
        return;
    }

    auto createInfo = populateDebugMessengerCreateInfo();
    if (const auto& result = validation::vkCreateDebugUtilsMessengerEXT(instance, createInfo.get(), nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::setupDebugMessenger: Failed to set up debug messenger, error code: {}.", kClassName, result));
    }
}

auto HelloTriangleApplication::populateDebugMessengerCreateInfo() -> std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT> {
    auto createInfo             = std::make_shared<VkDebugUtilsMessengerCreateInfoEXT>();
    createInfo->sType           = static_cast<VkStructureType>(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    createInfo->messageSeverity = static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) |
                                  static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) |
                                  static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);

    createInfo->messageType = static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) |
                              static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) |
                              static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);

    createInfo->pfnUserCallback = validation::debugCallback;
    createInfo->pUserData       = this;  // Optional - nullptr

    return createInfo;
}

void HelloTriangleApplication::createSurface() {
    if (const auto& result = glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createSurface: Failed to create window surface, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (0 == deviceCount) {
        throw std::runtime_error(std::format("{}::pickPhysicalDevice: Failed to find GPUs with Vulkan support!", kClassName));
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score.
    std::multimap<int32_t, VkPhysicalDevice> candidates;

    for (const auto& _device : devices) {
        uint32_t score = rateDeviceSuitability(_device);
        candidates.insert(std::make_pair(score, _device));
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error(std::format("{}::pickPhysicalDevice: Failed to find a suitable GPU!", kClassName));
    }

    for (auto candidate : candidates) {
        std::cout << std::format("{}::pickPhysicalDevice: Device score = {}.\n", kClassName, candidate.first);
    }
}

void HelloTriangleApplication::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos    = {};
    std::set<uint32_t>                   uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0F;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        // clang-format off
        VkDeviceQueueCreateInfo queueCreateInfo = { .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                    .pNext            = nullptr,
                                                    .flags            = {},
                                                    .queueFamilyIndex = queueFamily,
                                                    .queueCount       = 1,
                                                    .pQueuePriorities = &queuePriority };
        // clang-format on
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = { .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                      .pNext                   = nullptr,
                                      .flags                   = {},
                                      .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
                                      .pQueueCreateInfos       = queueCreateInfos.data(),
                                      .enabledLayerCount       = 0,        // Deprecated.
                                      .ppEnabledLayerNames     = nullptr,  // Deprecated.
                                      .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
                                      .ppEnabledExtensionNames = deviceExtensions.data(),
                                      .pEnabledFeatures        = &deviceFeatures };

    if (enableValidationLayers) {
        // Deprecated, but can be good to set anyways to be compatible with older implementations.
        createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if (const auto& result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createLogicalDevice: Failed to logical device, error code: {}.", kClassName, result));
    }

    // Retrieve the queue handles for each QueueFamily.
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

auto HelloTriangleApplication::rateDeviceSuitability(VkPhysicalDevice _device) -> uint32_t {
    uint32_t score = 0;

    QueueFamilyIndices      indices             = findQueueFamilies(_device);
    const bool              extensionSupported  = checkDeviceExtensionSupport(_device);
    SwapChainSupportDetails swapChainSupport    = querySwapChainSupport(_device);
    bool                    isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    // Device is not supported, return a score of 0.
    if (!indices.isComplete() || !extensionSupported || !isSwapChainAdequate) {
        return 0;
    }

    // Same queue family, boost the score.
    if (indices.graphicsFamily == indices.presentFamily) {
        score += DeviceSuitabilityScore::MEDIUM;
    }

    // Properties and features.
    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceFeatures   deviceFeatures   = {};
    vkGetPhysicalDeviceProperties(_device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(_device, &deviceFeatures);

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

auto HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice _device) -> HelloTriangleApplication::QueueFamilyIndices {
    QueueFamilyIndices indices          = {};
    uint32_t           queueFamilyCount = { 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

    uint32_t idx = 0;
    for (const auto& qFamily : queueFamilies) {
        auto presentSupport = static_cast<VkBool32>(false);
        vkGetPhysicalDeviceSurfaceSupportKHR(_device, idx, surface, &presentSupport);

        if (static_cast<bool>(presentSupport)) {
            indices.presentFamily = idx;
        }

        if (0 != (qFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.graphicsFamily = idx;
        }

        // Return if a queue family is found.
        if (indices.isComplete()) {
            return indices;
        }

        idx++;
    }

    return indices;
}

auto HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice _device) -> HelloTriangleApplication::SwapChainSupportDetails {
    SwapChainSupportDetails details = {};

    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, surface, &details.capabilities);

    // Surface formats (pixel format, color space)
    uint32_t formatCount = { 0 };
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface, &formatCount, nullptr);

    if (0 != formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface, &formatCount, details.formats.data());
    }

    // Available presentation modes
    uint32_t presentModeCount = { 0 };
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface, &presentModeCount, nullptr);

    if (0 != presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void HelloTriangleApplication::createSwapchain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR      surfaceFormat    = utilities::chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR        presentMode      = utilities::chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D              extent           = chooseSwapExtent(swapChainSupport.capabilities);

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

    QueueFamilyIndices       indices            = findQueueFamilies(physicalDevice);
    std::array<uint32_t, 2>  queueFamilyIndices = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    VkSwapchainCreateInfoKHR createInfo         = { .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                                    .pNext                 = nullptr,
                                                    .flags                 = {},
                                                    .surface               = surface,
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

    if (const auto& result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createSwapchain: Failed to create swap chain, error code: {}.", kClassName, result));
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent      = extent;
}

void HelloTriangleApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo { .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                           .pNext            = nullptr,
                                           .flags            = {},
                                           .image            = swapChainImages[i],
                                           .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                                           .format           = swapChainImageFormat,
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

        if (const auto& result = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error(std::format("{}::createImageViews: Failed to create image views, error code: {}.", kClassName, result));
        }
    }
}

auto HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int32_t width  = { 0 };
    int32_t height = { 0 };
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width      = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height     = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void HelloTriangleApplication::createRenderPass() {
    VkAttachmentDescription colorAttachment { .flags          = {},
                                              .format         = swapChainImageFormat,
                                              .samples        = VK_SAMPLE_COUNT_1_BIT,
                                              .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                              .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                              .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                              .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                              .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                                              .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

    VkAttachmentReference colorAttachmentRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = { .flags                   = {},
                                     .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     .inputAttachmentCount    = 0,
                                     .pInputAttachments       = nullptr,
                                     .colorAttachmentCount    = 1,
                                     .pColorAttachments       = &colorAttachmentRef,
                                     .pResolveAttachments     = nullptr,
                                     .pDepthStencilAttachment = nullptr,
                                     .preserveAttachmentCount = 0,
                                     .pPreserveAttachments    = nullptr };

    VkSubpassDependency dependency = { .srcSubpass      = VK_SUBPASS_EXTERNAL,
                                       .dstSubpass      = 0,
                                       .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       .srcAccessMask   = 0,
                                       .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                       .dependencyFlags = {} };

    VkRenderPassCreateInfo renderPassInfo = { .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                              .pNext           = nullptr,
                                              .flags           = {},
                                              .attachmentCount = 1,
                                              .pAttachments    = &colorAttachment,
                                              .subpassCount    = 1,
                                              .pSubpasses      = &subpass,
                                              .dependencyCount = 1,
                                              .pDependencies   = &dependency };

    if (const auto& result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createRenderPass: Failed to create render pass, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::createGraphicsPipeline() {
    std::filesystem::path shaderPath     = std::filesystem::current_path() += std::filesystem::path("/build/vulkan-triangle/src");
    auto                  vertShaderCode = utilities::readBinaryFile(std::filesystem::path(shaderPath / "triangle.vert.spv").string());
    auto                  fragShaderCode = utilities::readBinaryFile(std::filesystem::path(shaderPath / "triangle.frag.spv").string());

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = {},
        .stage               = VK_SHADER_STAGE_VERTEX_BIT,
        .module              = vertShaderModule,
        .pName               = "main",  // Entrypoint - can combine e.g. multiple modules.
        .pSpecializationInfo = nullptr  // Optional   - specify values for shader constants.
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
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
    VkPipelineDynamicStateCreateInfo dynamicState  = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = {},
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data()
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = {},
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // VkViewport viewport {
    //     .x = 0.0F,
    //     .y = 0.0F,
    //     .width = static_cast<float>(swapChainExtent.width),
    //     .height = static_cast<float>(swapChainExtent.height),
    //     .minDepth = 0.0F,
    //     .maxDepth = 1.0F
    // };

    // VkRect2D scissor = {
    //     .offset = {0, 0},
    //     .extent = swapChainExtent
    // };

    VkPipelineViewportStateCreateInfo viewportState = {
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

    VkPipelineRasterizationStateCreateInfo rasterizer {
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

    VkPipelineMultisampleStateCreateInfo multisampling = {
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

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
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

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = {},
        .logicOpEnable     = VK_FALSE,
        .logicOp           = VK_LOGIC_OP_COPY,           // Optional
        .attachmentCount   = 1,
        .pAttachments      = &colorBlendAttachment,
        .blendConstants    = { 0.0F, 0.0F, 0.0F, 0.0F }  // Optional
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .setLayoutCount         = 0,        // Optional
        .pSetLayouts            = nullptr,  // Optional
        .pushConstantRangeCount = 0,        // Optional
        .pPushConstantRanges    = nullptr   // Optional
    };
    // clang-format on

    if (const auto& result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createGraphicsPipeline: Failed to create pipeline layout, error code: {}.", kClassName, result));
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
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
        .layout              = pipelineLayout,
        .renderPass          = renderPass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,  // Optional
        .basePipelineIndex   = -1               // Optional
    };

    if (const auto& result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createGraphicsPipeline: Failed to create graphic pipeline, error code: {}.", kClassName, result));
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

auto HelloTriangleApplication::createShaderModule(const std::vector<char>& code) -> VkShaderModule {
    // clang-format off
    VkShaderModuleCreateInfo createInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = {},
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<const uint32_t*>(code.data())
    };
    // clang-format on

    VkShaderModule shaderModule {};
    if (const auto& result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createShaderModule: Failed to create shader module, error code: {}.", kClassName, result));
    }

    return shaderModule;
}

void HelloTriangleApplication::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 1> attachments     = { swapChainImageViews[i] };
        VkFramebufferCreateInfo    framebufferInfo = { .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                       .pNext           = nullptr,
                                                       .flags           = {},
                                                       .renderPass      = renderPass,
                                                       .attachmentCount = 1,
                                                       .pAttachments    = attachments.data(),
                                                       .width           = swapChainExtent.width,
                                                       .height          = swapChainExtent.height,
                                                       .layers          = 1 };

        if (const auto& result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error(std::format("{}::createFramebuffers: Failed to create framebuffer, error code: {}.", kClassName, result));
        }
    }
}

void HelloTriangleApplication::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo = { .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                         .pNext            = nullptr,
                                         .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                         .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value() };

    if (const auto& result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createCommandPool: Failed to create command pool, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::createCommandBuffers() {
    // clang-format off
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    // clang-format on

    if (const auto& result = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createCommandBuffers: Failed to create command buffer, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer _commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = {},      // Optional
        .pInheritanceInfo = nullptr  // Optional
    };

    if (const auto& result = vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::recordCommandBuffer: Failed to begin recording command buffer, error code: {}.", kClassName, result));
    }

    // clang-format off
    VkClearValue          clearColor     = { { { 0.0F, 0.0F, 0.0F, 1.0F } } };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = nullptr,
        .renderPass      = renderPass,
        .framebuffer     = swapChainFramebuffers[imageIndex],
        .renderArea {
            .offset      = { 0, 0 },
            .extent      = swapChainExtent
        },
        .clearValueCount = 1,
        .pClearValues    = &clearColor };
    // clang-format on

    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // clang-format off
    VkViewport viewport {
        .x = 0.0F,
        .y = 0.0F,
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth = 0.0F,
        .maxDepth = 1.0F
    };
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swapChainExtent
    };
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
    // clang-format on

    vkCmdDraw(_commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(_commandBuffer);

    if (const auto& result = vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::recordCommandBuffer: Failed to end command buffer, error code: {}.", kClassName, result));
    }
}

void HelloTriangleApplication::createSyncObjects() {
    // clang-format off
    VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = {} };
    VkFenceCreateInfo     fenceInfo     = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,     .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(    device, &fenceInfo,     nullptr, &inFlightFence)           != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::recordCommandBuffer: Failed to create semaphores/fence!", kClassName));
    }
    // clang-format on
}

void HelloTriangleApplication::checkExtensionSupport(const std::vector<const char*>& extensions) {
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
            throw std::runtime_error(std::format("{}::checkExtensionSupport: Missing required extension: [{}].", kClassName, extensionName));
        }
    }
}

auto HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice _device) -> bool {
    uint32_t extensionCount = { 0 };
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(static_cast<const char*>(extension.extensionName));
    }

    return requiredExtensions.empty();
}

void HelloTriangleApplication::checkValidationLayerSupport() {
    uint32_t availableCount { 0 };
    vkEnumerateInstanceLayerProperties(&availableCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(availableCount);
    vkEnumerateInstanceLayerProperties(&availableCount, availableLayers.data());

    for (const std::string layerName : validationLayers) {
        bool found { false };

        for (auto& availableLayer : availableLayers) {
            if (layerName == static_cast<const char*>(availableLayer.layerName)) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error(std::format("{}::checkValidationLayerSupport: Missing required layer [{}].", kClassName, layerName));
        }
    }
}

}  // namespace vt::triangle
