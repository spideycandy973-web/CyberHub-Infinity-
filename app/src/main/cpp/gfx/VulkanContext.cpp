#include "VulkanContext.h"
#include <vector>
#include <cstring>
#include <set>

namespace neon {

#ifndef NDEBUG
namespace {
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LOGE("VK: %s", data->pMessage);
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LOGW("VK: %s", data->pMessage);
    return VK_FALSE;
}
}
#endif

bool VulkanContext::init(ANativeWindow* window) {
    if (!createInstance()) return false;
    if (!createSurface(window)) return false;
    if (!pickPhysicalDevice()) return false;
    if (!createLogicalDevice()) return false;
    if (!createCommandPool()) return false;
    LOGI("VulkanContext ready");
    return true;
}

void VulkanContext::destroy() {
    if (mDevice) vkDeviceWaitIdle(mDevice);
    if (mCmdPool && mDevice)
        vkDestroyCommandPool(mDevice, mCmdPool, nullptr);
    if (mDebugMessenger && mInstance) {
        auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT"));
        if (fn) fn(mInstance, mDebugMessenger, nullptr);
    }
    if (mDevice)  vkDestroyDevice(mDevice, nullptr);
    if (mSurface && mInstance) vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    if (mInstance) vkDestroyInstance(mInstance, nullptr);
    mCmdPool = VK_NULL_HANDLE;
    mDebugMessenger = VK_NULL_HANDLE;
    mDevice = VK_NULL_HANDLE;
    mSurface = VK_NULL_HANDLE;
    mInstance = VK_NULL_HANDLE;
}

bool VulkanContext::createInstance() {
    std::vector<const char*> exts = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
    };
    std::vector<const char*> lay;

#ifndef NDEBUG
    uint32_t lc = 0;
    vkEnumerateInstanceLayerProperties(&lc, nullptr);
    std::vector<VkLayerProperties> layers(lc);
    if (lc) vkEnumerateInstanceLayerProperties(&lc, layers.data());
    for (auto& l : layers) {
        if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            lay.push_back("VK_LAYER_KHRONOS_validation");
            mValidation = true;
            LOGI("Validation layers enabled (debug build)");
            break;
        }
    }
#endif

    VkApplicationInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pApplicationName = "Neon District";
    ai.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    ai.pEngineName = "NeonEngine";
    ai.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    ai.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &ai;
    ci.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    ci.ppEnabledExtensionNames = exts.data();
    ci.enabledLayerCount = static_cast<uint32_t>(lay.size());
    ci.ppEnabledLayerNames = lay.data();
    VK_CHECK(vkCreateInstance(&ci, nullptr, &mInstance));

#ifndef NDEBUG
    if (mValidation) {
        VkDebugUtilsMessengerCreateInfoEXT dc{};
        dc.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dc.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dc.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dc.pfnUserCallback = debugCallback;
        auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT"));
        if (fn) fn(mInstance, &dc, nullptr, &mDebugMessenger);
    }
#endif
    return true;
}

bool VulkanContext::createSurface(ANativeWindow* window) {
    VkAndroidSurfaceCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    ci.window = window;
    VK_CHECK(vkCreateAndroidSurfaceKHR(mInstance, &ci, nullptr, &mSurface));
    return true;
}

bool VulkanContext::pickPhysicalDevice() {
    uint32_t dc = 0;
    vkEnumeratePhysicalDevices(mInstance, &dc, nullptr);
    if (!dc) { LOGE("No Vulkan devices"); return false; }
    std::vector<VkPhysicalDevice> devs(dc);
    vkEnumeratePhysicalDevices(mInstance, &dc, devs.data());

    for (auto dev : devs) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        if (props.apiVersion < VK_API_VERSION_1_1) continue;

        uint32_t qc = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qc, nullptr);
        std::vector<VkQueueFamilyProperties> qfs(qc);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qc, qfs.data());

        uint32_t gfx = UINT32_MAX, pres = UINT32_MAX;
        for (uint32_t i = 0; i < qc; ++i) {
            if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) gfx = i;
            VkBool32 p = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, mSurface, &p);
            if (p) pres = i;
        }
        if (gfx == UINT32_MAX || pres == UINT32_MAX) continue;

        uint32_t ec = 0;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &ec, nullptr);
        std::vector<VkExtensionProperties> exts(ec);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &ec, exts.data());
        bool swap = false;
        for (auto& e : exts)
            if (std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
                { swap = true; break; }
        if (!swap) continue;

        mPhysDevice = dev;
        mGfxFamily = gfx;
        mPresFamily = pres;

        mCaps.apiVersion = props.apiVersion;
        mCaps.deviceType = props.deviceType;
        mCaps.maxImageDimension2D = props.limits.maxImageDimension2D;
        mCaps.maxSamplerAnisotropy = props.limits.maxSamplerAnisotropy;
        mCaps.timestampPeriod = props.limits.timestampPeriod;

        VkPhysicalDeviceFeatures feats;
        vkGetPhysicalDeviceFeatures(dev, &feats);
        mCaps.supportsSamplerAnisotropy = feats.samplerAnisotropy;
        mCaps.supportsFillModeNonSolid  = feats.fillModeNonSolid;
        mCaps.supportsWideLines         = feats.wideLines;
        mCaps.supportsDepthClamp        = feats.depthClamp;
        mCaps.supportsTimestampQueries  = props.limits.timestampComputeAndGraphics;

        LOGI("GPU: %s  API %u.%u", props.deviceName,
             VK_VERSION_MAJOR(props.apiVersion),
             VK_VERSION_MINOR(props.apiVersion));
        return true;
    }
    LOGE("No suitable Vulkan 1.1+ device");
    return false;
}

bool VulkanContext::createLogicalDevice() {
    std::set<uint32_t> fams = {mGfxFamily, mPresFamily};
    float prio = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> qci;
    for (uint32_t f : fams) {
        VkDeviceQueueCreateInfo q{};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = f;
        q.queueCount = 1;
        q.pQueuePriorities = &prio;
        qci.push_back(q);
    }

    VkPhysicalDeviceFeatures feats{};
    if (mCaps.supportsSamplerAnisotropy) feats.samplerAnisotropy = VK_TRUE;
    if (mCaps.supportsFillModeNonSolid)  feats.fillModeNonSolid  = VK_TRUE;
    if (mCaps.supportsDepthClamp)        feats.depthClamp        = VK_TRUE;

    const char* devExt[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount = static_cast<uint32_t>(qci.size());
    ci.pQueueCreateInfos = qci.data();
    ci.pEnabledFeatures = &feats;
    ci.enabledExtensionCount = 1;
    ci.ppEnabledExtensionNames = devExt;
    VK_CHECK(vkCreateDevice(mPhysDevice, &ci, nullptr, &mDevice));

    vkGetDeviceQueue(mDevice, mGfxFamily,  0, &mGfxQueue);
    vkGetDeviceQueue(mDevice, mPresFamily, 0, &mPresQueue);
    return true;
}

bool VulkanContext::createCommandPool() {
    VkCommandPoolCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = mGfxFamily;
    VK_CHECK(vkCreateCommandPool(mDevice, &ci, nullptr, &mCmdPool));
    return true;
}

} // namespace neon
