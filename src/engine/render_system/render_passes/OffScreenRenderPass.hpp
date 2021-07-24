#pragma once

#include "RenderPass.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;

class OffScreenRenderPass final : public RenderPass {
public:
    
    explicit OffScreenRenderPass(
        uint32_t const imageWidth = 1024,
        uint32_t const imageHeight = 1024
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

    VkCommandBuffer GetCommandBuffer(RF::DrawPass const & drawPass) override;

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
    VkCommandBuffer mCommandBuffer {};

    RB::DepthImageGroup mDepthImageGroup {};
    
    uint32_t mImageWidth = 0;
    uint32_t mImageHeight = 0;

};

}
