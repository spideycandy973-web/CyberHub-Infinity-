#pragma once
#include "VulkanContext.h"
#include "Swapchain.h"
#include "ui/TextLayout.h"
#include <android/asset_manager.h>
#include <vector>

namespace neon {

class TextRenderer {
public:
    static constexpr uint32_t kMaxVertices = 16384;
    static constexpr int kFramesInFlight = 2;

    bool init(const VulkanContext& ctx, const Swapchain& swap, AAssetManager* assets);
    void destroy(const VulkanContext& ctx);
    void onSwapchainRecreated(const VulkanContext& ctx, const Swapchain& swap);

    void beginFrame();
    void drawText(const char* text, float x, float y, float scale,
                  Rotation rot, float screenW, float screenH);
    void record(VkCommandBuffer cmd, int frameIndex, float screenW, float screenH);

private:
    bool buildPipeline(const VulkanContext& ctx, const Swapchain& swap);
    bool createBuffers(const VulkanContext& ctx);
    void destroyPipeline(const VulkanContext& ctx);
    void destroyBuffers(const VulkanContext& ctx);

    AAssetManager* mAssets = nullptr;
    VkPipelineLayout mLayout   = VK_NULL_HANDLE;
    VkPipeline       mPipeline = VK_NULL_HANDLE;
    VkBuffer         mBuffers[kFramesInFlight]  = {};
    VkDeviceMemory   mMemories[kFramesInFlight] = {};
    void*            mMapped[kFramesInFlight]   = {};
    std::vector<TextVertex> mVertices;
};

} // namespace neon
