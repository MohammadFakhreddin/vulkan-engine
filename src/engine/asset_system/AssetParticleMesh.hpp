#pragma once

#include "AssetTypes.hpp"
#include "AssetBaseMesh.hpp"

namespace MFA::AssetSystem::Particle
{
    struct Vertex
    {
        Position localPosition{};
        int textureIndex = -1;
        UV uv{};
        Color color{};
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
