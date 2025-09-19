#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace vt::utilities {

static auto ReadBinaryFile(const std::string& filename) -> std::vector<char> {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Utilities::readBinaryFile: Failed to open file: [{}].", filename));
    }

    const size_t      fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<uint32_t>(fileSize));
    file.close();

    return buffer;
}

static auto ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) -> VkSurfaceFormatKHR {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // If this fails then it could be good to start ranking available formats,
    // but usually its ok to settle with the first format that is specified.
    return availableFormats[0];
}

static auto ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) -> VkPresentModeKHR {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // Fallback, only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available.
    return VK_PRESENT_MODE_FIFO_KHR;
}

}  // namespace vt::utilities
