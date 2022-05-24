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
        std::vector<std::string> const textureIds{};       // Texture local address
        std::vector<SamplerConfig> const samplerConfigs{};

        explicit Model(
            std::shared_ptr<MeshBase> mesh_,
            std::vector<std::string> textureIds_,
            std::vector<SamplerConfig> samplerConfigs_
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
