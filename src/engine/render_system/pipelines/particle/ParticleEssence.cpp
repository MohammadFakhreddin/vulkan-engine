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

    ParticleEssence::ParticleEssence(std::shared_ptr<AS::Model> const & cpuModel)
        : ParticleEssence(
            RF::CreateGpuModel(cpuModel.get(), "particle"),
            static_pointer_cast<Mesh>(cpuModel->mesh))
    {}
    
    //-------------------------------------------------------------------------------------------------
    // TODO: I do not like this constructor. I have to find a better interface for basePipeline.
    // TODO: Maybe having switch cases for each pipeline instead
    ParticleEssence::ParticleEssence(
        std::shared_ptr<RT::GpuModel> gpuModel,
        std::shared_ptr<Mesh> mesh
    )
        : EssenceBase(std::move(gpuModel))
        , mMesh(std::move(mesh))
        , mInstanceDataMemory(Memory::Alloc(mMesh->maxInstanceCount * sizeof(PerInstanceData)))
        , mInstancesData(mInstanceDataMemory->memory.as<PerInstanceData>())
    {
        mInstancesBuffers.resize(RF::GetMaxFramesPerFlight());
        for (auto & instancesBuffer : mInstancesBuffers)
        {
            instancesBuffer = RF::CreateVertexBuffer(mInstanceDataMemory->memory);
        }
    }

    //-------------------------------------------------------------------------------------------------

    ParticleEssence::~ParticleEssence() = default;

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::update(
        RT::CommandRecordState const & recordState,
        float deltaTimeInSec,
        VariantsList const & variants
    )
    {
        updateInstanceData(variants);
        updateVertexBuffer(recordState);
        updateInstanceBuffer(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::draw(
        RT::CommandRecordState const & recordState,
        float deltaTime
    )
    {
        bindVertexBuffer(recordState);
        bindInstanceBuffer(recordState);
        bindDescriptorSetGroup(recordState);
        // Draw
        RF::DrawIndexed(
            recordState,
            mMesh->getIndexCount(),
            mNextDrawInstanceCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::bindVertexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindVertexBuffer(
            recordState,
            *mGpuModel->meshBuffers->verticesBuffer[recordState.frameIndex],
            VERTEX_BUFFER_BIND_ID
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateInstanceData(VariantsList const & variants)
    {
        MFA_ASSERT(variants.size() < mMesh->maxInstanceCount);
        mNextDrawInstanceCount = std::max<uint32_t>(
            static_cast<uint32_t>(variants.size()),
            mMesh->maxInstanceCount
        );
        for (uint32_t i = 0; i < mNextDrawInstanceCount; ++i)
        {
            CAST_VARIANT(variants[i])->getWorldPosition(mInstancesData[i].instancePosition);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateInstanceBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::UpdateBuffer(
            *mInstancesBuffers[recordState.frameIndex],
            mInstanceDataMemory->memory
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::updateVertexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::UpdateBuffer(
            *mInstancesBuffers[recordState.frameIndex],
            mMesh->getVertexBuffer()->memory
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleEssence::bindInstanceBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindVertexBuffer(
            recordState,
            *mGpuModel->meshBuffers->verticesBuffer[recordState.frameIndex],
            INSTANCE_BUFFER_BIND_ID
        );
    }

    //-------------------------------------------------------------------------------------------------


}
