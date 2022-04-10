#include "ParticleEssence.hpp"

#include "ParticleVariant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"

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

        RF::UpdateLocalBuffer(
            commandBuffer,
            *mVertexBuffer->buffers[0],
            *outStageBuffer->buffers[0],
            vertexBlob
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

        RF::UpdateLocalBuffer(
            commandBuffer,
            *mParamsBuffer->buffers[0],
            *outStageBuffer->buffers[0],
            CBlobAliasOf(mParams)
        );
    }

    //-------------------------------------------------------------------------------------------------

    ParticleEssence::ParticleEssence(
        std::string nameId,
        Params params,
        uint32_t const maxInstanceCount,
        std::vector<std::shared_ptr<RT::GpuTexture>> textures
    )
        : EssenceBase(std::move(nameId))
        , mParams(std::move(params))
        , mMaxInstanceCount(maxInstanceCount)
        , mTextures(std::move(textures))
    {}

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::init(
        uint32_t const indexCount,
        CBlob const & vertexData,
        CBlob const & indexData
    )
    {

        mIndexCount = indexCount;

        {// Local buffers
            auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();

            std::shared_ptr<RT::BufferGroup> vertexStageBuffer = nullptr;
            createVertexBuffer(commandBuffer, vertexData, vertexStageBuffer);

            std::shared_ptr<RT::BufferGroup> indexStageBuffer = nullptr;
            createIndexBuffer(commandBuffer, indexData, indexStageBuffer);

            std::shared_ptr<RT::BufferGroup> paramsStageBuffer = nullptr;
            createParamsBuffer(commandBuffer, paramsStageBuffer);

            RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);
        }

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

    void ParticleEssence::compute(RT::CommandRecordState const & recordState) const
    {
        bindComputeDescriptorSet(recordState);
        RF::Dispatch(
            recordState,
            (mParams.count + 1) / 256,
            0,
            0
        );
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

    void ParticleEssence::updateParamsBuffer() const
    {
        auto const stageBuffer = RF::CreateStageBuffer(sizeof(mParams), 1);
        auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();
        RF::UpdateLocalBuffer(
            commandBuffer,
            *mParamsBuffer->buffers[0],
            *stageBuffer->buffers[0],
            CBlobAliasOf(mParams)
        );
        RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);
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
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            // -----------Textures------------
            MFA_ASSERT(mTextures.size() < maxTextureCount);
            std::vector<VkDescriptorImageInfo> imageInfos{};
            for (auto const & texture : mTextures)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = texture->imageView->imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            for (auto i = static_cast<uint32_t>(mTextures.size()); i < maxTextureCount; ++i)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
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
            RF::GetMaxFramesPerFlight(),
            descriptorSetLayout
        );

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            auto const & descriptorSet = mComputeDescriptorSet.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            // -----------Params-------------
            VkDescriptorBufferInfo paramsBufferInfo {
                .buffer = mParamsBuffer->buffers[0]->buffer,
                .offset = 0,
                .range = mParamsBuffer->bufferSize
            };
            descriptorSetSchema.AddUniformBuffer(&paramsBufferInfo);

            // -----------Particles-------------
            VkDescriptorBufferInfo particleBufferInfo {
                .buffer = mVertexBuffer->buffers[0]->buffer,
                .offset = 0,
                .range = mVertexBuffer->bufferSize
            };
            descriptorSetSchema.AddStorageBuffer(&particleBufferInfo);

            // --------------------------------
            descriptorSetSchema.UpdateDescriptorSets();
        }
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
            if (variant->IsVisible())
            {
                mShouldUpdate = true;
                return;
            }
        }
    }
    
    //-------------------------------------------------------------------------------------------------
    
}
