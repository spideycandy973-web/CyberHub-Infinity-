#pragma once
#include "VulkanContext.h"
#include <vector>

namespace neon {

class Swapchain {
public:
    bool create(const VulkanContext& ctx, uint32_t w, uint32_t h);
    void destroy(const VulkanContext& ctx);
    bool isStale() const { return mStale; }
    void markStale() { mStale = true; }

    VkSwapchainKHR handle()      const { return mSwapchain; }
    VkRenderPass   renderPass()  const { return mRenderPass; }
    VkFormat       format()      const { return mFormat; }
    VkExtent2D     extent()      const { return mExtent; }
    uint32_t       imageCount()  const { return static_cast<uint32_t>(mImages.size()); }
    VkImageView    imageView(uint32_t i)     const { return mImageViews[i]; }
    VkFramebuffer  framebuffer(uint32_t i)   const { return mFramebuffers[i]; }
    VkSemaphore    renderFinished(uint32_t i) const { return mRenderFinished[i]; }
    VkSurfaceTransformFlagBitsKHR preTransform() const { return mPreTransform; }

private:
    bool createRenderPass(const VulkanContext& ctx);
    bool createFramebuffers(const VulkanContext& ctx);
    bool createSemaphores(const VulkanContext& ctx);

    VkSwapchainKHR mSwapchain  = VK_NULL_HANDLE;
    VkRenderPass   mRenderPass = VK_NULL_HANDLE;
    VkFormat       mFormat     = VK_FORMAT_UNDEFINED;
    VkExtent2D     mExtent     = {0, 0};
    VkSurfaceTransformFlagBitsKHR mPreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    std::vector<VkImage>      mImages;
    std::vector<VkImageView>  mImageViews;
    std::vector<VkFramebuffer> mFramebuffers;
    std::vector<VkSemaphore>  mRenderFinished;
    bool mStale = false;
};

} // namespace neon
