#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include "core/Log.h"
#include <vector>

#define VK_CHECK(call)                                                     \
    do {                                                                   \
        VkResult _r = (call);                                              \
        if (_r != VK_SUCCESS) {                                            \
            LOGE("Vulkan error %d at %s:%d", (int)_r, __FILE__, __LINE__); \
            return false;                                                  \
        }                                                                  \
    } while (0)

namespace neon {

inline uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t typeFilter,
                               VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem;
    vkGetPhysicalDeviceMemoryProperties(phys, &mem);
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (mem.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return UINT32_MAX;
}

inline VkShaderModule createShaderModule(VkDevice device,
                                         const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size() * 4;
    ci.pCode = code.data();
    VkShaderModule mod = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &ci, nullptr, &mod);
    return mod;
}

} // namespace neon
