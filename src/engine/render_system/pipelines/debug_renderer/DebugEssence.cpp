#include "DebugEssence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    DebugEssence::DebugEssence(
        std::string const & nameId,
        AS::Debug::Mesh const & mesh
    )
        : EssenceBase(nameId)
        , mIndicesCount(mesh.getIndexCount())
    {
        std::shared_ptr<RT::BufferGroup> vertexStageBuffer = nullptr;
        std::shared_ptr<RT::BufferGroup> indexStageBuffer = nullptr;
        
        auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();
        prepareVertexBuffer(commandBuffer, mesh, vertexStageBuffer);
        prepareIndexBuffer(commandBuffer, mesh, indexStageBuffer);
        RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    DebugEssence::~DebugEssence() = default;

    //-------------------------------------------------------------------------------------------------

    uint32_t DebugEssence::getIndicesCount() const
    {
        return mIndicesCount;
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::bindForGraphicPipeline(RT::CommandRecordState const & recordState) const
    {
        bindVertexBuffer(recordState);
        bindIndexBuffer(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::prepareVertexBuffer(
        VkCommandBuffer commandBuffer,
        Mesh const & mesh,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        auto const vertexData = mesh.getVertexData()->memory;

        outStageBuffer = RF::CreateStageBuffer(vertexData.len, 1);

        mVerticesBuffer = RF::CreateVertexBuffer(
            commandBuffer,
            *outStageBuffer->buffers[0],
            vertexData
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::prepareIndexBuffer(
        VkCommandBuffer commandBuffer,
        Mesh const & mesh,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        auto const indexData = mesh.getIndexData()->memory;
        outStageBuffer = RF::CreateStageBuffer(indexData.len, 1);
        mIndicesBuffer = RF::CreateIndexBuffer(commandBuffer, *outStageBuffer->buffers[0], indexData);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::bindVertexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindVertexBuffer(recordState, *mVerticesBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::bindIndexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindIndexBuffer(recordState, *mIndicesBuffer);
    }

    //-------------------------------------------------------------------------------------------------

}
