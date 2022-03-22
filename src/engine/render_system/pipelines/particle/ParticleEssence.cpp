#include "ParticleEssence.hpp"

#include "ParticleVariant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"

#define CAST_VARIANT(variant) static_pointer_cast<ParticleVariant>(variant)

namespace MFA
{
    using namespace AS::Particle;

    static constexpr uint32_t VERTEX_BUFFER_BIND_ID = 0;
    static constexpr uint32_t INSTANCE_BUFFER_BIND_ID = 1;

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::BufferGroup> createVertexBuffer(
        VkCommandBuffer commandBuffer,
        CBlob const & vertexBlob,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        auto bufferGroup = RF::CreateBufferGroup(
            vertexBlob.len,
            1,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        outStageBuffer = RF::CreateStageBuffer(vertexBlob.len, 1);

        RF::UpdateLocalBuffer(
            commandBuffer,
            *bufferGroup->buffers[0],
            *outStageBuffer->buffers[0],
            vertexBlob
        );

        return bufferGroup;
    }

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::BufferAndMemory> createIndexBuffer(
        VkCommandBuffer commandBuffer,
        CBlob const & indexBlob,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        outStageBuffer = RF::CreateStageBuffer(indexBlob.len, 1);

        return RF::CreateIndexBuffer(
            commandBuffer,
            *outStageBuffer->buffers[0],
            indexBlob
        );
    }

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::BufferGroup> createInstanceBuffer(size_t const size)
    {
        return RF::CreateUniformBuffer(
            size,
            1,
            RF::MemoryFlags::hostVisible
        );
    }

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::BufferGroup> createParamsBuffer(
        VkCommandBuffer commandBuffer,
        Params const & params,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        outStageBuffer = RF::CreateStageBuffer(sizeof(params), 1);

        auto paramsBuffer = RF::CreateUniformBuffer(
            sizeof(params),
            1,
            RF::MemoryFlags::local
        );

        RF::UpdateLocalBuffer(
            commandBuffer,
            *paramsBuffer->buffers[0],
            *outStageBuffer->buffers[0],
            CBlobAliasOf(params)
        );

        return paramsBuffer;
    }

    //-------------------------------------------------------------------------------------------------

    ParticleEssence::ParticleEssence(
        std::string nameId,
        uint32_t maxInstanceCount,
        std::vector<std::shared_ptr<RT::GpuTexture>> textures
    )
        : EssenceBase(std::move(nameId))
        , mMaxInstanceCount(maxInstanceCount)
        , mTextures(std::move(textures))
    {}

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::init(
        uint32_t const indexCount,
        Params const & params,
        CBlob const & vertexData,
        CBlob const & indexData
    )
    {

        mIndexCount = indexCount;

        {// Local buffers
            auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();

            std::shared_ptr<RT::BufferGroup> vertexStageBuffer = nullptr;
            mVertexBuffer = createVertexBuffer(commandBuffer, vertexData, vertexStageBuffer);

            std::shared_ptr<RT::BufferGroup> indexStageBuffer = nullptr;
            mIndexBuffer = createIndexBuffer(commandBuffer, indexData, indexStageBuffer);

            std::shared_ptr<RT::BufferGroup> paramsStageBuffer = nullptr;
            mParamsBuffer = createParamsBuffer(commandBuffer, params, paramsStageBuffer);

            RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);
        }

        {// Host visible buffer
            auto const instanceSize = mMaxInstanceCount * sizeof(PerInstanceData);
            mInstanceDataMemory = Memory::Alloc(instanceSize);
            mInstanceBuffer = createInstanceBuffer(instanceSize);
        }

        mIsInitialized = true;
        
    }

    //-------------------------------------------------------------------------------------------------

    ParticleEssence::~ParticleEssence() = default;

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::update(VariantsList const & variants)
    {
        checkIfUpdateIsRequired(variants);
        if (mShouldUpdate)
        {
            updateInstanceData(variants);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::render(RT::CommandRecordState const & recordState) const
    {
        if (mShouldUpdate)
        {
            updateInstanceBuffer(recordState);
        }

        // TODO: We need barrier here for vertex buffer

        bindVertexBuffer(recordState);
        bindInstanceBuffer(recordState);
        bindIndexBuffer(recordState);
        bindGraphicDescriptorSet(recordState);
        // Draw
        RF::DrawIndexed(
            recordState,
            mIndexCount,
            mNextDrawInstanceCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateParamsBuffer(Params const & params) const
    {
        auto const stageBuffer = RF::CreateStageBuffer(sizeof(params), 1);
        auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();
        RF::UpdateLocalBuffer(
            commandBuffer,
            *mParamsBuffer->buffers[0],
            *stageBuffer->buffers[0],
            CBlobAliasOf(params)
        );
        RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::bindVertexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindVertexBuffer(
            recordState,
            *mVertexBuffer->buffers[0],
            VERTEX_BUFFER_BIND_ID
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateInstanceData(VariantsList const & variants)
    {
        MFA_ASSERT(variants.size() <= mMaxInstanceCount);
        mNextDrawInstanceCount = std::min<uint32_t>(
            static_cast<uint32_t>(variants.size()),
            mMaxInstanceCount
        );

        auto const * instanceData = mInstanceDataMemory->memory.as<PerInstanceData *>();

        uint32_t visibleVariantCount = 0;
        for (uint32_t i = 0; i < mNextDrawInstanceCount; ++i)
        {
            auto const & variant = CAST_VARIANT(variants[i]);
            if (variant->IsVisible())
            {
                variant->getWorldPosition(instanceData[visibleVariantCount]->instancePosition);
                ++visibleVariantCount;
            }
        }
        mNextDrawInstanceCount = visibleVariantCount;
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateInstanceBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::UpdateHostVisibleBuffer(
            *mInstancesBuffer->buffers[recordState.frameIndex],
            mInstanceDataMemory->memory
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::bindInstanceBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindVertexBuffer(
            recordState,
            *mInstanceBuffer->buffers[recordState.frameIndex],
            INSTANCE_BUFFER_BIND_ID
        );
    }

    //-------------------------------------------------------------------------------------------------
    
    void ParticleEssence::bindIndexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindIndexBuffer(
            recordState,
            *mIndexBuffer
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::bindGraphicDescriptorSet(RT::CommandRecordState const & recordState) const
    {
        RF::AutoBindDescriptorSet(
            recordState,
            RF::UpdateFrequency::PerEssence,
            mGraphicDescriptorSet
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::bindComputeDescriptorSet(RT::CommandRecordState const & recordState) const
    {
        RF::BindDescriptorSet(
            recordState,
            RF::UpdateFrequency::PerEssence,
            mGraphicDescriptorSet.descriptorSets[0]
        );
    }

    //-------------------------------------------------------------------------------------------------
    
    void ParticleEssence::checkIfUpdateIsRequired(VariantsList const & variants)
    {
        mShouldUpdate = false;
        for (auto const & variant : variants)
        {
            if (variant->IsVisible())
            {
                mShouldUpdate = true;
                return;
            }
        }
    }
    
    //-------------------------------------------------------------------------------------------------
    
}
