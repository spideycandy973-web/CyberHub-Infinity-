#include "Swapchain.h"
#include <algorithm>

namespace neon {

bool Swapchain::create(const VulkanContext& ctx, uint32_t w, uint32_t h) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physDevice(), ctx.surface(), &caps);

    uint32_t fc = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physDevice(), ctx.surface(), &fc, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fc);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physDevice(), ctx.surface(), &fc, fmts.data());
    if (fmts.empty()) { LOGE("No surface formats"); return false; }

    VkSurfaceFormatKHR chosen = fmts[0];
    for (auto& f : fmts) {
        if ((f.format == VK_FORMAT_R8G8B8A8_UNORM ||
             f.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }
    mFormat = chosen.format;

    if (caps.currentExtent.width != UINT32_MAX) {
        mExtent = caps.currentExtent;
    } else {
        mExtent.width  = std::clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width);
        mExtent.height = std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    if (mExtent.width == 0 || mExtent.height == 0) return false;

    uint32_t imgCount = std::max(3u, caps.minImageCount);
    if (caps.maxImageCount > 0) imgCount = std::min(imgCount, caps.maxImageCount);

    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
        if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        else if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            alpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        else
            alpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }

    mPreTransform = caps.currentTransform;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = ctx.surface();
    ci.minImageCount = imgCount;
    ci.imageFormat = mFormat;
    ci.imageColorSpace = chosen.colorSpace;
    ci.imageExtent = mExtent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform = mPreTransform;
    ci.compositeAlpha = alpha;
    ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    ci.clipped = VK_TRUE;

    uint32_t fams[] = {ctx.gfxFamily(), ctx.presFamily()};
    if (ctx.gfxFamily() != ctx.presFamily()) {
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = fams;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    VK_CHECK(vkCreateSwapchainKHR(ctx.device(), &ci, nullptr, &mSwapchain));

    uint32_t ac = 0;
    vkGetSwapchainImagesKHR(ctx.device(), mSwapchain, &ac, nullptr);
    mImages.resize(ac);
    vkGetSwapchainImagesKHR(ctx.device(), mSwapchain, &ac, mImages.data());

    mImageViews.resize(ac);
    for (uint32_t i = 0; i < ac; ++i) {
        VkImageViewCreateInfo vci{};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = mImages[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = mFormat;
        vci.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                          VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_CHECK(vkCreateImageView(ctx.device(), &vci, nullptr, &mImageViews[i]));
    }

    if (!createRenderPass(ctx)) return false;
    if (!createFramebuffers(ctx)) return false;
    if (!createSemaphores(ctx)) return false;
    mStale = false;
    LOGI("Swapchain %ux%u  %u images  transform=%u",
         mExtent.width, mExtent.height, ac, mPreTransform);
    return true;
}

void Swapchain::destroy(const VulkanContext& ctx) {
    if (!ctx.device()) return;
    vkDeviceWaitIdle(ctx.device());
    for (auto s : mRenderFinished)  vkDestroySemaphore(ctx.device(), s, nullptr);
    for (auto f : mFramebuffers)    vkDestroyFramebuffer(ctx.device(), f, nullptr);
    for (auto v : mImageViews)      vkDestroyImageView(ctx.device(), v, nullptr);
    if (mRenderPass)  vkDestroyRenderPass(ctx.device(), mRenderPass, nullptr);
    if (mSwapchain)   vkDestroySwapchainKHR(ctx.device(), mSwapchain, nullptr);
    mRenderFinished.clear(); mFramebuffers.clear();
    mImageViews.clear(); mImages.clear();
    mRenderPass = VK_NULL_HANDLE; mSwapchain = VK_NULL_HANDLE;
}

bool Swapchain::createRenderPass(const VulkanContext& ctx) {
    VkAttachmentDescription att{};
    att.format = mFormat;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &ref;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = 1; ci.pAttachments = &att;
    ci.subpassCount = 1;    ci.pSubpasses = &sub;
    ci.dependencyCount = 1; ci.pDependencies = &dep;
    VK_CHECK(vkCreateRenderPass(ctx.device(), &ci, nullptr, &mRenderPass));
    return true;
}

bool Swapchain::createFramebuffers(const VulkanContext& ctx) {
    mFramebuffers.resize(mImages.size());
    for (size_t i = 0; i < mImages.size(); ++i) {
        VkFramebufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass = mRenderPass;
        ci.attachmentCount = 1;
        ci.pAttachments = &mImageViews[i];
        ci.width = mExtent.width;
        ci.height = mExtent.height;
        ci.layers = 1;
        VK_CHECK(vkCreateFramebuffer(ctx.device(), &ci, nullptr, &mFramebuffers[i]));
    }
    return true;
}

bool Swapchain::createSemaphores(const VulkanContext& ctx) {
    mRenderFinished.resize(mImages.size());
    VkSemaphoreCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (size_t i = 0; i < mImages.size(); ++i)
        VK_CHECK(vkCreateSemaphore(ctx.device(), &ci, nullptr, &mRenderFinished[i]));
    return true;
}

} // namespace neon
