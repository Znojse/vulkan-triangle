#include <cstdlib>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "hello_triangle_application.hpp"

namespace vt::triangle {

void HelloTriangleApplication::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::createInstance() {
    VkApplicationInfo appInfo {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t     glfwExtensionCount    = 0;
    const char** glfwExtensions        = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount   = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount       = 0;

    if (!glfwExtensions) {
        throw std::runtime_error("HelloTriangleApplication::createInstance: Failed to get GLFW required instance extensions.");
    }

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        auto requiredExt = std::string(glfwExtensions[i]);
        bool found       = false;

        std::cout << std::format("Comparing required extension [{}] with available extension:", requiredExt) << std::endl;
        for (uint32_t j = 0; j < extensionCount; j++) {
            std::string availableExt { extensions[j].extensionName };
            std::cout << std::format("\t[{}]", availableExt) << std::endl;

            if (requiredExt.compare(std::string(availableExt)) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            std::cout << "Available extension:" << std::endl;
            for (const auto& extension : extensions) {
                std::cout << std::format("\t [{}]", extension.extensionName) << std::endl;
            }
            throw std::runtime_error(std::format("HelloTriangleApplication::createInstance: Missing required Vulkanextension: [{}].", requiredExt));
        }
    }

    if (auto result = vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error(std::format("HelloTriangleApplication::createInstance: Failed to create instancewith error code: {}.", result).c_str());
    }
}

void HelloTriangleApplication::initVulkan() { createInstance(); }

void HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::cleanup() {
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

}  // namespace vt::triangle
