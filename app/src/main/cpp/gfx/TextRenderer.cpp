#include "TextRenderer.h"
#include <algorithm>
#include <cstring>

namespace neon {

namespace {
std::vector<uint32_t> loadSpirv(AAssetManager* mgr, const char* path) {
    AAsset* a = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
    if (!a) { LOGE("Cannot open asset %s", path); return {}; }
    size_t len = AAsset_getLength(a);
    std::vector<uint32_t> buf(len / 4);
    AAsset_read(a, buf.data(), len);
    AAsset_close(a);
    return buf;
}
}

bool TextRenderer::init(const VulkanContext& ctx, const Swapchain& swap,
                        AAssetManager* assets) {
    mAssets = assets;
    if (!buildPipeline(ctx, swap)) return false;
    if (!createBuffers(ctx)) return false;
    mVertices.reserve(kMaxVertices);
    LOGI("TextRenderer ready");
    return true;
}

void TextRenderer::destroy(const VulkanContext& ctx) {
    destroyPipeline(ctx);
    destroyBuffers(ctx);
}

void TextRenderer::onSwapchainRecreated(const VulkanContext& ctx,
                                         const Swapchain& swap) {
    destroyPipeline(ctx);
    buildPipeline(ctx, swap);
}

void TextRenderer::beginFrame() {
    mVertices.clear();
}

void TextRenderer::drawText(const char* text, float x, float y, float scale,
                            Rotation rot, float screenW, float screenH) {
    layoutText(text, x, y, scale, rot, screenW, screenH, mVertices);
}

void TextRenderer::record(VkCommandBuffer cmd, int frameIndex,
                          float screenW, float screenH) {
    if (mVertices.empty()) return;
    uint32_t count = std::min(static_cast<uint32_t>(mVertices.size()), kMaxVertices);
    std::memcpy(mMapped[frameIndex], mVertices.data(), count * sizeof(TextVertex));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &mBuffers[frameIndex], &off);

    struct Push { float color[4]; float screenInfo[4]; } push;
    push.color[0] = 0.0f;  push.color[1] = 0.9f;
    push.color[2] = 1.0f;  push.color[3] = 1.0f;
    push.screenInfo[0] = screenW; push.screenInfo[1] = screenH;
    push.screenInfo[2] = 0.0f;   push.screenInfo[3] = 0.0f;

    vkCmdPushConstants(cmd, mLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(Push), &push);
    vkCmdDraw(cmd, count, 1, 0, 0);
}

bool TextRenderer::buildPipeline(const VulkanContext& ctx, const Swapchain& swap) {
    auto vSpv = loadSpirv(mAssets, "shaders/text.vert.spv");
    auto fSpv = loadSpirv(mAssets, "shaders/text.frag.spv");
    if (vSpv.empty() || fSpv.empty()) return false;

    VkShaderModule vMod = createShaderModule(ctx.device(), vSpv);
    VkShaderModule fMod = createShaderModule(ctx.device(), fSpv);
    if (!vMod || !fMod) {
        if (vMod) vkDestroyShaderModule(ctx.device(), vMod, nullptr);
        if (fMod) vkDestroyShaderModule(ctx.device(), fMod, nullptr);
        LOGE("Shader module creation failed");
        return false;
    }

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.size = 32;

    VkPipelineLayoutCreateInfo lci{};
    lci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    lci.pushConstantRangeCount = 1;
    lci.pPushConstantRanges = &pcr;
    if (vkCreatePipelineLayout(ctx.device(), &lci, nullptr, &mLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(ctx.device(), vMod, nullptr);
        vkDestroyShaderModule(ctx.device(), fMod, nullptr);
        LOGE("Pipeline layout creation failed");
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vMod; stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fMod; stages[1].pName = "main";

    VkVertexInputBindingDescription bind{};
    bind.binding = 0; bind.stride = sizeof(TextVertex);
    bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription attr{};
    attr.location = 0; attr.binding = 0;
    attr.format = VK_FORMAT_R32G32_SFLOAT; attr.offset = 0;

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
    vi.vertexAttributeDescriptionCount = 1; vi.pVertexAttributeDescriptions = &attr;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport vp{0, 0, (float)swap.extent().width, (float)swap.extent().height, 0, 1};
    VkRect2D sc{{0,0}, swap.extent()};
    VkPipelineViewportStateCreateInfo vps{};
    vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vps.viewportCount = 1; vps.pViewports = &vp;
    vps.scissorCount = 1;  vps.pScissors = &sc;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1; cb.pAttachments = &cba;

    VkGraphicsPipelineCreateInfo gci{};
    gci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gci.stageCount = 2; gci.pStages = stages;
    gci.pVertexInputState = &vi;
    gci.pInputAssemblyState = &ia;
    gci.pViewportState = &vps;
    gci.pRasterizationState = &rs;
    gci.pMultisampleState = &ms;
    gci.pColorBlendState = &cb;
    gci.layout = mLayout;
    gci.renderPass = swap.renderPass();

    VkResult r = vkCreateGraphicsPipelines(ctx.device(), VK_NULL_HANDLE,
                                           1, &gci, nullptr, &mPipeline);
    vkDestroyShaderModule(ctx.device(), vMod, nullptr);
    vkDestroyShaderModule(ctx.device(), fMod, nullptr);
    if (r != VK_SUCCESS) {
        vkDestroyPipelineLayout(ctx.device(), mLayout, nullptr);
        mLayout = VK_NULL_HANDLE;
        LOGE("Pipeline creation failed %d", r);
        return false;
    }
    return true;
}

bool TextRenderer::createBuffers(const VulkanContext& ctx) {
    VkDeviceSize sz = kMaxVertices * sizeof(TextVertex);
    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = sz;
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    for (int i = 0; i < kFramesInFlight; ++i) {
        VK_CHECK(vkCreateBuffer(ctx.device(), &bci, nullptr, &mBuffers[i]));
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(ctx.device(), mBuffers[i], &req);
        uint32_t mt = findMemoryType(ctx.physDevice(), req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (mt == UINT32_MAX) return false;
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = req.size; ai.memoryTypeIndex = mt;
        VK_CHECK(vkAllocateMemory(ctx.device(), &ai, nullptr, &mMemories[i]));
        VK_CHECK(vkBindBufferMemory(ctx.device(), mBuffers[i], mMemories[i], 0));
        VK_CHECK(vkMapMemory(ctx.device(), mMemories[i], 0, sz, 0, &mMapped[i]));
    }
    return true;
}

void TextRenderer::destroyPipeline(const VulkanContext& ctx) {
    if (mPipeline) vkDestroyPipeline(ctx.device(), mPipeline, nullptr);
    if (mLayout)   vkDestroyPipelineLayout(ctx.device(), mLayout, nullptr);
    mPipeline = VK_NULL_HANDLE; mLayout = VK_NULL_HANDLE;
}

void TextRenderer::destroyBuffers(const VulkanContext& ctx) {
    for (int i = 0; i < kFramesInFlight; ++i) {
        if (mMapped[i])   vkUnmapMemory(ctx.device(), mMemories[i]);
        if (mBuffers[i])  vkDestroyBuffer(ctx.device(), mBuffers[i], nullptr);
        if (mMemories[i]) vkFreeMemory(ctx.device(), mMemories[i], nullptr);
        mMapped[i] = nullptr;
        mBuffers[i] = mMemories[i] = VK_NULL_HANDLE;
    }
}

} // namespace neon
