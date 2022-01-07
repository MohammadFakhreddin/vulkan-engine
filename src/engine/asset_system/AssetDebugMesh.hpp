#pragma once

#include "AssetTypes.hpp"
#include "AssetBaseMesh.hpp"

namespace MFA::AssetSystem::Debug
{
    struct Vertex
    {
        Position position{};
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
