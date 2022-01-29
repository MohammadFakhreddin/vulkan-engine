#pragma once

#include "AssetTexture.hpp"
#include "AssetBaseMesh.hpp"

#include <memory>
#include <vector>

namespace MFA::AssetSystem 
{
    struct Model
    {
        std::shared_ptr<MeshBase> const mesh{};
        std::vector<std::shared_ptr<Texture>> const textures{};

        explicit Model(
            std::shared_ptr<MeshBase> mesh_,
            std::vector<std::shared_ptr<Texture>> textures_
        );
        ~Model();

        Model(Model const &) noexcept = delete;
        Model(Model &&) noexcept = delete;
        Model & operator= (Model const & rhs) noexcept = delete;
        Model & operator= (Model && rhs) noexcept = delete;
    };

}

namespace MFA
{
    namespace AS = AssetSystem;
}
