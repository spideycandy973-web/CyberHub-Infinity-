#pragma once
#include "VulkanContext.h"
#include "Swapchain.h"
#include "TextRenderer.h"
#include <android/native_window.h>
#include <android/asset_manager.h>

namespace neon {

class Renderer {
public:
    bool init(ANativeWindow* window, AAssetManager* assets);
    void destroy();
    bool render(float dt);
    bool surfaceLost() const { return mSurfaceLost; }

private:
    bool createSync();
    void destroySync();
    bool recreateSwapchain();
    Rotation getRotation() const;

    VulkanContext mCtx;
    Swapchain     mSwap;
    TextRenderer  mText;

    static constexpr int kFrames = 2;
    VkCommandBuffer mCmd[kFrames]   = {};
    VkFence         mFence[kFrames] = {};
    VkSemaphore     mAvail[kFrames] = {};

    int   mFrame    = 0;
    float mTime     = 0;
    float mFpsAccum = 0;
    int   mFpsCount = 0;
    int   mFpsShow  = 0;
    float mMsShow   = 0;
    bool  mSurfaceLost = false;
    ANativeWindow* mWindow = nullptr;
};

} // namespace neon
