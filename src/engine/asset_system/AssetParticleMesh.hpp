#pragma once

#include "AssetBaseMesh.hpp"

namespace MFA::AssetSystem::Particle
{
    struct Vertex
    {
        float localPosition[3]{};
        int textureIndex = -1;
        float uv [2]{};
        float color [3]{};
        float alpha = 1.0f;
        // These variable currently used only on cpu and not on gpu
        float remainingLifeInSec = 0.0f; 
        float initialLocalPosition[3] {};
    };

    struct PerInstanceData
    {
        float instancePosition[3] {};
    };

    class Mesh final : public MeshBase
    {
    public:

        explicit Mesh(uint32_t maxInstanceCount_);
        ~Mesh() override;

        Mesh(Mesh const &) noexcept = delete;
        Mesh(Mesh &&) noexcept = delete;
        Mesh & operator= (Mesh const & rhs) noexcept = delete;
        Mesh & operator= (Mesh && rhs) noexcept = delete;

        uint32_t const maxInstanceCount;

    };

}

namespace MFA
{
    namespace AS = AssetSystem;
}
