#pragma once

#include "AssetTypes.hpp"
#include "AssetBaseMesh.hpp"

namespace MFA::AssetSystem::Particle
{
    struct Vertex
    {
        Position position{};
        int textureIndex = -1;
        UV uv{};
        Color color{};
    };

    class Mesh final : public MeshBase
    {
    public:

        explicit Mesh();
        ~Mesh() override;

        Mesh(Mesh const &) noexcept = delete;
        Mesh(Mesh &&) noexcept = delete;
        Mesh & operator= (Mesh const & rhs) noexcept = delete;
        Mesh & operator= (Mesh && rhs) noexcept = delete;

    };

}

namespace MFA
{
    namespace AS = AssetSystem;
}
