#include "AssetModel.hpp"

namespace MFA::AssetSystem
{
    //-------------------------------------------------------------------------------------------------

    Model::Model(
        std::shared_ptr<MeshBase> mesh_,
        std::vector<std::string> textureIds_,
        std::vector<SamplerConfig> samplerConfigs_
    )
        : mesh(std::move(mesh_))
        , textureIds(std::move(textureIds_))
        , samplerConfigs(std::move(samplerConfigs_))
    {}

    //-------------------------------------------------------------------------------------------------

    Model::~Model() = default;

    //-------------------------------------------------------------------------------------------------
}

