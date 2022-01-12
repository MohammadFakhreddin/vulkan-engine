#include "AssetParticleMesh.hpp"

#include "engine/BedrockAssert.hpp"

#include "engine/render_system/RenderFrontend.hpp"

namespace MFA::AssetSystem::Particle
{
    //-------------------------------------------------------------------------------------------------

    Mesh::Mesh(uint32_t const maxInstanceCount_)
        : MeshBase(RF::GetMaxFramesPerFlight())
        , maxInstanceCount(maxInstanceCount_)
    {
        MFA_ASSERT(maxInstanceCount > 0);
    }

    //-------------------------------------------------------------------------------------------------

    Mesh::~Mesh() = default;

    //-------------------------------------------------------------------------------------------------
}
