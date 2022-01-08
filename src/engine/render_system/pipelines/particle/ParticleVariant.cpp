#include "ParticleVariant.hpp"

#include "ParticleEssence.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/asset_system/AssetParticleMesh.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{
    using namespace AS::Particle;
    
    //-------------------------------------------------------------------------------------------------

    ParticleVariant::ParticleVariant(ParticleEssence const * essence)
        : VariantBase(essence)
        , mIndicesCount(essence->getIndicesCount())
    {}

    //-------------------------------------------------------------------------------------------------

    ParticleVariant::~ParticleVariant() = default;

    //-------------------------------------------------------------------------------------------------

    bool ParticleVariant::getTransform(float outTransform[16]) const
    {
        if (auto const ptr = mTransformComponent.lock())
        {
            Matrix::CopyGlmToCells(ptr->GetTransform(), outTransform);
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------------

    void ParticleVariant::Draw(RT::CommandRecordState const & recordState) const
    {
        RF::DrawIndexed(recordState, mIndicesCount);
    }

    //-------------------------------------------------------------------------------------------------

}
