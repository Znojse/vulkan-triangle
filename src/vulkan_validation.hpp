#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <span>
#include <sstream>
#include <string>

namespace vt::validation {

// clang-format off
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name, readability-identifier-naming)
auto inline vkCreateDebugUtilsMessengerEXT(VkInstance                                instance,
                                           const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                           const VkAllocationCallbacks*              pAllocator,
                                           VkDebugUtilsMessengerEXT*                 pDebugMessenger) -> VkResult {
    // clang-format on
    auto pFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (nullptr != pFunc) {
        return pFunc(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void inline DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto pFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (nullptr != pFunc) {
        pFunc(instance, debugMessenger, pAllocator);
    }
}

static auto GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT severity) -> std::string {
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return "Verbose";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return "Info";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return "Warning";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return "Error";
        default:
            std::stringstream severityStream {};
            severityStream << "Invalid type code: " << severity << "\n";
            return severityStream.str();
    }
}

static auto GetDebugTypeStr(VkDebugUtilsMessageTypeFlagsEXT type) -> std::string {
    switch (type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return "General";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            return "Validation";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            return "Performance";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
            return "Device address binding";
        default:
            std::stringstream typeStream {};
            typeStream << "Invalid type code: " << type << "\n";
            return typeStream.str();
    }
}

// clang-format off
static VKAPI_ATTR auto VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                void*                                       /*pUserData*/) -> VkBool32 {
    // clang-format on
    std::stringstream errMsg = {};
    errMsg << "-----------------------------------------------\n";
    errMsg << "Vulkan-Validation::debugCallback: \n" << pCallbackData->pMessage << "\n\n";
    errMsg << "\tSeverity: " << GetDebugSeverityStr(messageSeverity) << "\n";
    errMsg << "\tType: " << GetDebugTypeStr(messageType) << "\n";
    errMsg << "\tObjects: ";

    const std::span<const VkDebugUtilsObjectNameInfoEXT> callbackObjectSpan = { pCallbackData->pObjects, pCallbackData->objectCount };
    for (const auto& object : callbackObjectSpan) {
        errMsg << std::hex << object.objectHandle << " ";
    }

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enough to show
        std::cerr << errMsg.str() << "\n";
    }

    return VK_FALSE;
}

}  // namespace vt::validation
