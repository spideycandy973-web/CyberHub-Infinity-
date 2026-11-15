#include "Renderer.h"
#include <cmath>
#include <cstdio>

namespace neon {

bool Renderer::init(ANativeWindow* window, AAssetManager* assets) {
    mWindow = window;
    mSurfaceLost = false;

    if (!mCtx.init(window)) return false;

    int32_t ww = ANativeWindow_getWidth(window);
    int32_t hh = ANativeWindow_getHeight(window);
    if (ww <= 0 || hh <= 0) {
        LOGE("Invalid window size %dx%d", ww, hh);
        mCtx.destroy();
        return false;
    }

    if (!mSwap.create(mCtx, static_cast<uint32_t>(ww), static_cast<uint32_t>(hh))) {
        mCtx.destroy();
        return false;
    }
    if (!mText.init(mCtx, mSwap, assets)) {
        mSwap.destroy(mCtx);
        mCtx.destroy();
        return false;
    }

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = mCtx.cmdPool();
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = kFrames;
    if (vkAllocateCommandBuffers(mCtx.device(), &ai, mCmd) != VK_SUCCESS) {
        mText.destroy(mCtx);
        mSwap.destroy(mCtx);
        mCtx.destroy();
        return false;
    }

    if (!createSync()) {
        vkFreeCommandBuffers(mCtx.device(), mCtx.cmdPool(), kFrames, mCmd);
        mText.destroy(mCtx);
        mSwap.destroy(mCtx);
        mCtx.destroy();
        return false;
    }

    mFrame = 0; mTime = 0; mFpsAccum = 0; mFpsCount = 0;
    LOGI("Renderer ready");
    return true;
}

void Renderer::destroy() {
    if (mCtx.device()) vkDeviceWaitIdle(mCtx.device());
    destroySync();
    mText.destroy(mCtx);
    mSwap.destroy(mCtx);
    if (mCtx.cmdPool() && mCtx.device())
        vkFreeCommandBuffers(mCtx.device(), mCtx.cmdPool(), kFrames, mCmd);
    mCtx.destroy();
}

Rotation Renderer::getRotation() const {
    switch (mSwap.preTransform()) {
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:  return Rotation::Rot90;
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR: return Rotation::Rot180;
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR: return Rotation::Rot270;
        default: return Rotation::None;
    }
}

bool Renderer::render(float dt) {
    if (mSurfaceLost) return false;

    if (mSwap.isStale()) {
        if (!recreateSwapchain()) return true;
    }

    mTime += dt;
    mFpsAccum += dt;
    mFpsCount++;
    if (mFpsAccum >= 0.5f) {
        mFpsShow = static_cast<int>(mFpsCount / mFpsAccum);
        mMsShow  = (mFpsAccum / mFpsCount) * 1000.0f;
        mFpsAccum = 0; mFpsCount = 0;
    }

    vkWaitForFences(mCtx.device(), 1, &mFence[mFrame], VK_TRUE, UINT64_MAX);

    uint32_t img = 0;
    VkResult acq = vkAcquireNextImageKHR(mCtx.device(), mSwap.handle(),
                   UINT64_MAX, mAvail[mFrame], VK_NULL_HANDLE, &img);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR)  { mSwap.markStale(); return true; }
    if (acq == VK_ERROR_SURFACE_LOST_KHR) { mSurfaceLost = true; return false; }
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR) return false;

    vkResetFences(mCtx.device(), 1, &mFence[mFrame]);
    vkResetCommandBuffer(mCmd[mFrame], 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(mCmd[mFrame], &bi);

    float t = mTime * 0.5f;
    VkClearValue clr;
    clr.color.float32[0] = 0.5f + 0.5f * std::sin(t);
    clr.color.float32[1] = 0.5f + 0.5f * std::sin(t + 2.094f);
    clr.color.float32[2] = 0.5f + 0.5f * std::sin(t + 4.189f);
    clr.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo rpi{};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpi.renderPass = mSwap.renderPass();
    rpi.framebuffer = mSwap.framebuffer(img);
    rpi.renderArea = {{0,0}, mSwap.extent()};
    rpi.clearValueCount = 1;
    rpi.pClearValues = &clr;
    vkCmdBeginRenderPass(mCmd[mFrame], &rpi, VK_SUBPASS_CONTENTS_INLINE);

    float sw = static_cast<float>(mSwap.extent().width);
    float sh = static_cast<float>(mSwap.extent().height);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "FPS %d  %.1f MS", mFpsShow, mMsShow);

    Rotation rot = getRotation();

    mText.beginFrame();
    mText.drawText(buf, 8.0f, 8.0f, 2.0f, rot, sw, sh);
    mText.record(mCmd[mFrame], mFrame, sw, sh);

    vkCmdEndRenderPass(mCmd[mFrame]);
    vkEndCommandBuffer(mCmd[mFrame]);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore rFin = mSwap.renderFinished(img);
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;  si.pWaitSemaphores = &mAvail[mFrame];
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;  si.pCommandBuffers = &mCmd[mFrame];
    si.signalSemaphoreCount = 1; si.pSignalSemaphores = &rFin;
    VK_CHECK(vkQueueSubmit(mCtx.graphicsQueue(), 1, &si, mFence[mFrame]));

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &rFin;
    pi.swapchainCount = 1;
    VkSwapchainKHR sc = mSwap.handle();
    pi.pSwapchains = &sc; pi.pImageIndices = &img;
    VkResult pres = vkQueuePresentKHR(mCtx.presentQueue(), &pi);
    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR)
        mSwap.markStale();
    else if (pres == VK_ERROR_SURFACE_LOST_KHR)
        { mSurfaceLost = true; return false; }

    mFrame = (mFrame + 1) % kFrames;
    return true;
}

bool Renderer::createSync() {
    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (int i = 0; i < kFrames; ++i) {
        VK_CHECK(vkCreateFence(mCtx.device(), &fci, nullptr, &mFence[i]));
        VK_CHECK(vkCreateSemaphore(mCtx.device(), &sci, nullptr, &mAvail[i]));
    }
    return true;
}

void Renderer::destroySync() {
    for (int i = 0; i < kFrames; ++i) {
        if (mFence[i]) vkDestroyFence(mCtx.device(), mFence[i], nullptr);
        if (mAvail[i]) vkDestroySemaphore(mCtx.device(), mAvail[i], nullptr);
        mFence[i] = VK_NULL_HANDLE;
        mAvail[i] = VK_NULL_HANDLE;
    }
}

bool Renderer::recreateSwapchain() {
    vkDeviceWaitIdle(mCtx.device());
    mSwap.destroy(mCtx);
    int32_t ww = ANativeWindow_getWidth(mWindow);
    int32_t hh = ANativeWindow_getHeight(mWindow);
    if (ww <= 0 || hh <= 0) return false;
    if (!mSwap.create(mCtx, static_cast<uint32_t>(ww), static_cast<uint32_t>(hh)))
        return false;
    mText.onSwapchainRecreated(mCtx, mSwap);
    return true;
}

} // namespace neon
