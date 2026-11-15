#pragma once
#include "VkUtil.h"
#include <android/native_window.h>

namespace neon {

struct DeviceCaps {
    uint32_t apiVersion = 0;
    VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
    uint32_t maxImageDimension2D = 4096;
    float maxSamplerAnisotropy = 1.0f;
    float timestampPeriod = 1.0f;
    bool supportsSamplerAnisotropy = false;
    bool supportsFillModeNonSolid = false;
    bool supportsWideLines = false;
    bool supportsDepthClamp = false;
    bool supportsTimestampQueries = false;
};

class VulkanContext {
public:
    bool init(ANativeWindow* window);
    void destroy();

    VkInstance       instance()      const { return mInstance; }
    VkPhysicalDevice physDevice()    const { return mPhysDevice; }
    VkDevice         device()        const { return mDevice; }
    VkQueue          graphicsQueue() const { return mGfxQueue; }
    VkQueue          presentQueue()  const { return mPresQueue; }
    uint32_t         gfxFamily()     const { return mGfxFamily; }
    uint32_t         presFamily()    const { return mPresFamily; }
    VkSurfaceKHR     surface()       const { return mSurface; }
    VkCommandPool    cmdPool()       const { return mCmdPool; }
    const DeviceCaps& caps()         const { return mCaps; }

private:
    bool createInstance();
    bool createSurface(ANativeWindow* window);
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createCommandPool();

    VkInstance       mInstance   = VK_NULL_HANDLE;
    VkSurfaceKHR     mSurface    = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysDevice = VK_NULL_HANDLE;
    VkDevice         mDevice     = VK_NULL_HANDLE;
    VkQueue          mGfxQueue   = VK_NULL_HANDLE;
    VkQueue          mPresQueue  = VK_NULL_HANDLE;
    uint32_t         mGfxFamily  = UINT32_MAX;
    uint32_t         mPresFamily = UINT32_MAX;
    VkCommandPool    mCmdPool    = VK_NULL_HANDLE;
    DeviceCaps       mCaps;
    [[maybe_unused]] bool mValidation = false;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
};

} // namespace neon
