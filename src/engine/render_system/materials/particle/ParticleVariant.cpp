#include "ParticleVariant.hpp"

#include "ParticleEssence.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/asset_system/AssetParticleMesh.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"

namespace MFA
{
    using namespace AS::Particle;
    
    //-------------------------------------------------------------------------------------------------

    ParticleVariant::ParticleVariant(ParticleEssence const * essence)
        : VariantBase(essence)
    {}

    //-------------------------------------------------------------------------------------------------

    ParticleVariant::~ParticleVariant() = default;

    //-------------------------------------------------------------------------------------------------

    bool ParticleVariant::getWorldPosition(float outWorldPosition[3]) const
    {
        if (auto const ptr = mTransformComponent.lock())
        {
            auto const & worldPosition = ptr->GetWorldPosition();
            ::memcpy(outWorldPosition, &worldPosition, sizeof(float) * 3);
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------------

}
