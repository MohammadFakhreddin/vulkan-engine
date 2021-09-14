#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA {

class DisplayRenderPass;

class ShadowRenderPass final : public RenderPass {
public:
    
    explicit ShadowRenderPass(
        uint32_t const imageWidth = 2048,
        uint32_t const imageHeight = 2048
    )
        : mImageWidth(imageWidth)
        , mImageHeight(imageHeight)
    {}
    ~ShadowRenderPass() override = default;

    ShadowRenderPass (ShadowRenderPass const &) noexcept = delete;
    ShadowRenderPass (ShadowRenderPass &&) noexcept = delete;
    ShadowRenderPass & operator = (ShadowRenderPass const &) noexcept = delete;
    ShadowRenderPass & operator = (ShadowRenderPass &&) noexcept = delete;

    VkRenderPass GetVkRenderPass() override;

    [[nodiscard]]
    RT::DepthImageGroup const & GetDepthImageGroup() const;
    
protected:

    void internalInit() override;

    void internalShutdown() override;

    void internalBeginRenderPass(RT::DrawPass & drawPass) override;

    void internalEndRenderPass(RT::DrawPass & drawPass) override;

    void internalResize() override;

private:

    void createRenderPass();

    void createFrameBuffer(VkExtent2D const & shadowExtent);

    inline static constexpr VkFormat SHADOW_MAP_FORMAT = VK_FORMAT_R32_SFLOAT;

    VkRenderPass mVkRenderPass {};
    VkFramebuffer mFrameBuffer {};
    
    RT::DepthImageGroup mDepthImageGroup {};
    
    uint32_t mImageWidth = 0;
    uint32_t mImageHeight = 0;

    DisplayRenderPass * mDisplayRenderPass = nullptr;
};

}
