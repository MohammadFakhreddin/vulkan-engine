#pragma once

#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/asset_system/AssetDebugMesh.hpp"

namespace MFA
{
    class DebugEssence final : public EssenceBase
    {
    public:

        using Mesh = AS::Debug::Mesh;

        explicit DebugEssence(
            std::string const & nameId,
            Mesh const & mesh
        );
        ~DebugEssence() override;
        
        DebugEssence & operator= (DebugEssence && rhs) noexcept = delete;
        DebugEssence (DebugEssence const &) noexcept = delete;
        DebugEssence (DebugEssence && rhs) noexcept = delete;
        DebugEssence & operator = (DebugEssence const &) noexcept = delete;

        [[nodiscard]]
        uint32_t getIndicesCount() const;

        void bindForGraphicPipeline(RT::CommandRecordState const & recordState) const;

    private:

        void prepareVertexBuffer(
            VkCommandBuffer commandBuffer,
            Mesh const & mesh,
            std::shared_ptr<RT::BufferGroup> & outStageBuffer
        );

        void prepareIndexBuffer(
            VkCommandBuffer commandBuffer,
            Mesh const & mesh,
            std::shared_ptr<RT::BufferGroup> & outStageBuffer
        );

        void bindVertexBuffer(RT::CommandRecordState const & recordState) const;

        void bindIndexBuffer(RT::CommandRecordState const & recordState) const;

        std::shared_ptr<RT::BufferGroup> mVerticesBuffer {};

        std::shared_ptr<RT::BufferGroup> mIndicesBuffer {};

        uint32_t const mIndicesCount;

    };
}
