#pragma once

#include "RenderPass.hpp"

namespace MFA {

class DisplayRenderPass;

namespace RF = RenderFrontend;
namespace RB = RenderBackend;

class OffScreenRenderPass final : public RenderPass {
public:
    
    explicit OffScreenRenderPass(
        uint32_t const imageWidth = 2048,
        uint32_t const imageHeight = 2048
    )
        : mImageWidth(imageWidth)
        , mImageHeight(imageHeight)
    {}
    ~OffScreenRenderPass() override = default;

    OffScreenRenderPass (OffScreenRenderPass const &) noexcept = delete;
    OffScreenRenderPass (OffScreenRenderPass &&) noexcept = delete;
    OffScreenRenderPass & operator = (OffScreenRenderPass const &) noexcept = delete;
    OffScreenRenderPass & operator = (OffScreenRenderPass &&) noexcept = delete;

    VkRenderPass GetVkRenderPass() override;

    [[nodiscard]]
    RB::DepthImageGroup const & GetDepthImageGroup() const;

    //[[nodiscard]]
    //RB::ColorImageGroup const & GetColorImageGroup() const;

protected:

    void internalInit() override;

    void internalShutdown() override;

    void internalBeginRenderPass(RF::DrawPass const & drawPass) override;

    void internalEndRenderPass(RF::DrawPass const & drawPass) override;

    void internalResize() override;

private:

    void createRenderPass();

    void createFrameBuffer(VkExtent2D const & shadowExtent);

    inline static constexpr VkFormat SHADOW_MAP_FORMAT = VK_FORMAT_R32_SFLOAT;

    VkRenderPass mVkRenderPass {};
    VkFramebuffer mFrameBuffer {};
    
    RB::DepthImageGroup mDepthImageGroup {};
    
    uint32_t mImageWidth = 0;
    uint32_t mImageHeight = 0;

    DisplayRenderPass * mDisplayRenderPass = nullptr;

    //RB::ColorImageGroup mColorImageGroup {};
};

}
