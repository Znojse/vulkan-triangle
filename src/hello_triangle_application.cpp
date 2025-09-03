#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <print>
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

    if (func != nullptr) {
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

    if (auto result = vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error(std::format("{}::createInstance: Failed to create instancewith error code: {}.", kClassName, result).c_str());
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

void HelloTriangleApplication::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    auto createInfo = populateDebugMessengerCreateInfo();
    if (vkCreateDebugUtilsMessengerEXT(instance, createInfo.get(), nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("{}::setupDebugMessenger: Failed to set up debug messenger!");
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

void HelloTriangleApplication::checkExtensionSupport(const std::vector<const char*>& extensions) {
    uint32_t availableCount { 0 };
    vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(availableCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, availableExtensions.data());

    for (std::string extensionName : extensions) {
        bool found { false };

        for (auto& availableExtensions : availableExtensions) {
            if (extensionName.compare(availableExtensions.extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error(std::format("{}::checkExtensionSupport: Missing required extension: [{}].", kClassName, extensionName));
        }
    }
}

void HelloTriangleApplication::checkValidationLayerSupport() {
    uint32_t availableCount { 0 };
    vkEnumerateInstanceLayerProperties(&availableCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(availableCount);
    vkEnumerateInstanceLayerProperties(&availableCount, availableLayers.data());

    for (std::string layerName : validationLayers) {
        bool found { false };

        for (auto& availableLayer : availableLayers) {
            if (layerName.compare(availableLayer.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error(std::format("{}::createInstance: Missing required layer [{}].", kClassName, layerName));
        }
    }
}

}  // namespace vt::triangle
