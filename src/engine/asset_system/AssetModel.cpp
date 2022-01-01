#include "AssetModel.hpp"

namespace MFA::AssetSystem
{
    //-------------------------------------------------------------------------------------------------

    Model::Model(
        std::shared_ptr<MeshBase> mesh_,
        std::vector<std::shared_ptr<Texture>> textures_
    )
        : mesh(std::move(mesh_))
        , textures(std::move(textures_))
    {}

    //-------------------------------------------------------------------------------------------------

    Model::~Model() = default;

    //-------------------------------------------------------------------------------------------------
}

