#include <algorithm>
#include <cstdint>
#include <format>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "hello_triangle_application.hpp"

// clang-format off
VkResult vkCreateDebugUtilsMessengerEXT(VkInstance                                instance,
                                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks*              pAllocator,
                                        VkDebugUtilsMessengerEXT*                 pDebugMessenger) {
    // clang-format on
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (func) {
        func(instance, debugMessenger, pAllocator);
    }
}

// clang-format off
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void*                                       /*pUserData*/) {
    // clang-format on
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        // Message is important enough to show
        std::cerr << "validation layer: " << messageType << " - " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

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
}

void HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::cleanup() {
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
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

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() {
    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (!glfwExtensions) {
        throw std::runtime_error(std::format("{}::getRequiredExtensions: Failed to get GLFW required instance extensions.", kClassName));
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void HelloTriangleApplication::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    auto createInfo = populateDebugMessengerCreateInfo();
    if (const auto& result = vkCreateDebugUtilsMessengerEXT(instance, createInfo.get(), nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::setupDebugMessenger: Failed to set up debug messenger, error code: {}.", kClassName, result));
    }
}

std::shared_ptr<VkDebugUtilsMessengerCreateInfoEXT> HelloTriangleApplication::populateDebugMessengerCreateInfo() {
    // clang-format off
    auto createInfo                                = std::make_shared<VkDebugUtilsMessengerCreateInfoEXT>();
    createInfo->sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo->messageType                        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                                                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo->pfnUserCallback                    = debugCallback;
    createInfo->pUserData                          = this;  // Optional - nullptr
    // clang-format on

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

    for (const auto& device : devices) {
        int32_t score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error(std::format("{}::pickPhysicalDevice: Failed to find a suitable GPU!", kClassName));
    }

    for (auto c : candidates) {
        std::cout << std::format("{}::pickPhysicalDevice: Device score = {}.", kClassName, c.first) << std::endl;
    }
}

void HelloTriangleApplication::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos    = {};
    std::set<uint32_t>                   uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        // clang-format off
        VkDeviceQueueCreateInfo queueCreateInfo = { .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                    .pNext            = nullptr,
                                                    .flags            = {},
                                                    .queueFamilyIndex = queueFamily,
                                                    .queueCount       = 1,
                                                    .pQueuePriorities = &queuePriority };
        // clang-format on
        queueCreateInfos.push_back(std::move(queueCreateInfo));
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

int32_t HelloTriangleApplication::rateDeviceSuitability(VkPhysicalDevice device) {
    int32_t score = 0;

    QueueFamilyIndices      indices             = findQueueFamilies(device);
    const bool              extensionSupported  = checkDeviceExtensionSupport(device);
    SwapChainSupportDetails swapChainSupport    = querySwapChainSupport(device);
    bool                    isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    // Device is not supported, return a score of 0.
    if (!indices.isComplete() || !extensionSupported || !isSwapChainAdequate) return 0;

    // Same queue family, boost the score.
    if (indices.graphicsFamily.value() == indices.presentFamily.value()) {
        score += 500;
    }

    // Properties and features.
    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceFeatures   deviceFeatures   = {};
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Discrete GPUs have a significant performance advantage.
    if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == deviceProperties.deviceType) {
        score += 1000;
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

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices          = {};
    uint32_t           queueFamilyCount = { 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int16_t i = 0;
    for (const auto& qFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (qFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // Return if a queue family is found.
        if (indices.isComplete()) {
            return indices;
        }

        i++;
    }

    return indices;
}

HelloTriangleApplication::SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details = {};

    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Surface formats (pixel format, color space)
    uint32_t formatCount = { 0 };
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (0 != formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Available presentation modes
    uint32_t presentModeCount = { 0 };
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (0 != presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void HelloTriangleApplication::createSwapchain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR      surfaceFormat    = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR        presentMode      = chooseSwapPresentMode(swapChainSupport.presentModes);
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

    QueueFamilyIndices       indices              = findQueueFamilies(physicalDevice);
    uint32_t                 queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    VkSwapchainCreateInfoKHR createInfo           = { .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
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
                                                      .pQueueFamilyIndices   = queueFamilyIndices,
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
        VkImageViewCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .image = swapChainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapChainImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        if (const auto& result = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error(std::format("{}::createImageViews: Failed to create image views, error code: {}.", kClassName, result));
        }
    }
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // If this fails then it could be good to start ranking available formats,
    // but usually its ok to settle with the first format that is specified.
    return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // Fallback, only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available.
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int32_t width, height = { 0 };
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width      = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height     = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void HelloTriangleApplication::checkExtensionSupport(const std::vector<const char*>& extensions) {
    uint32_t availableCount { 0 };
    vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(availableCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, availableExtensions.data());

    for (std::string extensionName : extensions) {
        bool found { false };

        for (auto& availableExtensions : availableExtensions) {
            if (0 == extensionName.compare(availableExtensions.extensionName)) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error(std::format("{}::checkExtensionSupport: Missing required extension: [{}].", kClassName, extensionName));
        }
    }
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount = { 0 };
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void HelloTriangleApplication::checkValidationLayerSupport() {
    uint32_t availableCount { 0 };
    vkEnumerateInstanceLayerProperties(&availableCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(availableCount);
    vkEnumerateInstanceLayerProperties(&availableCount, availableLayers.data());

    for (std::string layerName : validationLayers) {
        bool found { false };

        for (auto& availableLayer : availableLayers) {
            if (0 == layerName.compare(availableLayer.layerName)) {
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
