#include "ParticleEssence.hpp"

#include "ParticleVariant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"

#include <cmath>

#define CAST_VARIANT(variant) static_pointer_cast<ParticleVariant>(variant)

namespace MFA
{
    using namespace AS::Particle;

    static constexpr uint32_t VERTEX_BUFFER_BIND_ID = 0;
    static constexpr uint32_t INSTANCE_BUFFER_BIND_ID = 1;

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::createVertexBuffer(
        VkCommandBuffer commandBuffer,
        CBlob const & vertexBlob,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        mVertexBuffer = RF::CreateBufferGroup(
            vertexBlob.len,
            1,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        outStageBuffer = RF::CreateStageBuffer(vertexBlob.len, 1);

        RF::UpdateHostVisibleBuffer(
            *outStageBuffer->buffers[0],
            vertexBlob
        );

        RF::UpdateLocalBuffer(
            commandBuffer,
            *mVertexBuffer->buffers[0],
            *outStageBuffer->buffers[0]
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::createIndexBuffer(
        VkCommandBuffer commandBuffer,
        CBlob const & indexBlob,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        outStageBuffer = RF::CreateStageBuffer(indexBlob.len, 1);

        mIndexBuffer =  RF::CreateIndexBuffer(
            commandBuffer,
            *outStageBuffer->buffers[0],
            indexBlob
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::createInstanceBuffer()
    {
        auto const bufferSize = mMaxInstanceCount * sizeof(PerInstanceData);
        mInstanceData = Memory::Alloc(bufferSize);
        mInstanceBuffer = RF::CreateBufferGroup(
            bufferSize,
            RF::GetMaxFramesPerFlight(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::createParamsBuffer(
        VkCommandBuffer commandBuffer,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    )
    {
        outStageBuffer = RF::CreateStageBuffer(sizeof(mParams), 1);

        mParamsBuffer = RF::CreateLocalUniformBuffer(sizeof(mParams), 1);

        RF::UpdateHostVisibleBuffer(
            *outStageBuffer->buffers[0],
            CBlobAliasOf(mParams)
        );

        RF::UpdateLocalBuffer(
            commandBuffer,
            *mParamsBuffer->buffers[0],
            *outStageBuffer->buffers[0]
        );
    }

    //-------------------------------------------------------------------------------------------------

    ParticleEssence::ParticleEssence(
        std::string nameId,
        Params const & params,
        uint32_t const maxInstanceCount,
        std::vector<std::shared_ptr<RT::GpuTexture>> textures
    )
        : EssenceBase(std::move(nameId))
        , mParams(params)
        , mMaxInstanceCount(maxInstanceCount)
        , mTextures(std::move(textures))
    {}

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::init(CBlob const & vertexData, CBlob const & indexData)
    {

        //--------------Local buffers-----------
        auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();

        std::shared_ptr<RT::BufferGroup> vertexStageBuffer = nullptr;
        createVertexBuffer(commandBuffer, vertexData, vertexStageBuffer);

        std::shared_ptr<RT::BufferGroup> indexStageBuffer = nullptr;
        createIndexBuffer(commandBuffer, indexData, indexStageBuffer);

        std::shared_ptr<RT::BufferGroup> paramsStageBuffer = nullptr;
        createParamsBuffer(commandBuffer, paramsStageBuffer);

        RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);
        //---------------------------------------

        createInstanceBuffer();

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

    void ParticleEssence::preComputeBarrier(std::vector<VkBufferMemoryBarrier> & outBarrier) const
    {
        VkBufferMemoryBarrier barrier =
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			nullptr,
			0,
			VK_ACCESS_SHADER_WRITE_BIT,
			RF::GetGraphicQueueFamily(),
			RF::GetComputeQueueFamily(),
			mVertexBuffer->buffers[0]->buffer,
			0,
			mVertexBuffer->buffers[0]->size
		};

        outBarrier.emplace_back(barrier);
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::compute(RT::CommandRecordState const & recordState)
    {
        if (mShouldUpdate == false)
        {
            return;
        }

        if (mIsParamsBufferDirty)
        {
            updateParamsBuffer(recordState);
            mIsParamsBufferDirty = false;
        }

        bindComputeDescriptorSet(recordState);
        auto const dispatchCount = static_cast<uint32_t>(std::ceil(static_cast<float>(mParams.count) / 256.0f));
        RF::Dispatch(
            recordState,
            dispatchCount,
            1,
            1
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::preRenderBarrier(std::vector<VkBufferMemoryBarrier> & outBarrier) const
    {
        VkBufferMemoryBarrier barrier =
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			nullptr,
			VK_ACCESS_SHADER_WRITE_BIT,
			0,
			RF::GetComputeQueueFamily(),
			RF::GetGraphicQueueFamily(),
			mVertexBuffer->buffers[0]->buffer,
			0,
			mVertexBuffer->buffers[0]->size
		};
        outBarrier.emplace_back(barrier);
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::render(RT::CommandRecordState const & recordState) const
    {
        if (mShouldUpdate == false)
        {
            return;
        }

        updateInstanceBuffer(recordState);

        bindVertexBuffer(recordState);
        bindInstanceBuffer(recordState);
        bindIndexBuffer(recordState);
        bindGraphicDescriptorSet(recordState);

        // Draw
        RF::DrawIndexed(
            recordState,
            mParams.count,
            mNextDrawInstanceCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::notifyParamsBufferUpdated()
    {
        mIsParamsBufferDirty = true;
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::createGraphicDescriptorSet(
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        RT::GpuTexture const & errorTexture,
        uint32_t const maxTextureCount
    )
    {
        mGraphicDescriptorSet = RF::CreateDescriptorSets(
            descriptorPool,
            RF::GetMaxFramesPerFlight(),
            descriptorSetLayout
        );

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            auto const & descriptorSet = mGraphicDescriptorSet.descriptorSets[frameIndex];
            MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            // -----------Textures------------
            MFA_ASSERT(mTextures.size() < maxTextureCount);
            std::vector<VkDescriptorImageInfo> imageInfos{};
            for (auto const & texture : mTextures)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = VK_NULL_HANDLE,
                    .imageView = texture->imageView->imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            for (auto i = static_cast<uint32_t>(mTextures.size()); i < maxTextureCount; ++i)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = VK_NULL_HANDLE,
                    .imageView = errorTexture.imageView->imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            MFA_ASSERT(imageInfos.size() == maxTextureCount);
            descriptorSetSchema.AddImage(
                imageInfos.data(),
                static_cast<uint32_t>(imageInfos.size())
            );
            // --------------------------------
            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::createComputeDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
    {
        mComputeDescriptorSet = RF::CreateDescriptorSets(
            descriptorPool,
            1,
            descriptorSetLayout
        );

        auto const & descriptorSet = mComputeDescriptorSet.descriptorSets[0];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        DescriptorSetSchema descriptorSetSchema{ descriptorSet };

        // -----------Params-------------
        VkDescriptorBufferInfo const paramsBufferInfo {
            .buffer = mParamsBuffer->buffers[0]->buffer,
            .offset = 0,
            .range = mParamsBuffer->bufferSize
        };
        descriptorSetSchema.AddUniformBuffer(&paramsBufferInfo);

        // -----------Particles-------------
        VkDescriptorBufferInfo const particleBufferInfo {
            .buffer = mVertexBuffer->buffers[0]->buffer,
            .offset = 0,
            .range = mVertexBuffer->bufferSize
        };
        descriptorSetSchema.AddStorageBuffer(&particleBufferInfo);

        // --------------------------------
        descriptorSetSchema.UpdateDescriptorSets();
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

        auto * instanceData = mInstanceData->memory.as<PerInstanceData>();

        uint32_t visibleVariantCount = 0;
        for (uint32_t i = 0; i < mNextDrawInstanceCount; ++i)
        {
            auto const & variant = CAST_VARIANT(variants[i]);
            if (variant->IsVisible())
            {
                variant->getWorldPosition(instanceData[visibleVariantCount].instancePosition);
                ++visibleVariantCount;
            }
        }
        mNextDrawInstanceCount = visibleVariantCount;
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateInstanceBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::UpdateHostVisibleBuffer(
            *mInstanceBuffer->buffers[recordState.frameIndex],
            mInstanceData->memory
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
            mComputeDescriptorSet.descriptorSets[0]
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::checkIfUpdateIsRequired(VariantsList const & variants)
    {
        mShouldUpdate = false;
        for (auto const & variant : variants)
        {
            // TODO: We need an occlusion test for particles as well
            if (variant->IsVisible())
            {
                mShouldUpdate = true;
                return;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateParamsBuffer(RT::CommandRecordState const & recordState)
    {
        if (mParamsStageBuffer == nullptr)
        {
            mParamsStageBuffer = RF::CreateStageBuffer(sizeof(mParams), 1);
        }
        RF::UpdateHostVisibleBuffer(*mParamsStageBuffer->buffers[0], CBlobAliasOf(mParams));
        RF::UpdateLocalBuffer(
            recordState.commandBuffer,
            *mParamsBuffer->buffers[0],
            *mParamsStageBuffer->buffers[0]
        );
    }

    //-------------------------------------------------------------------------------------------------

}
