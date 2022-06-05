#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <memory>
#include <string>

#include "engine/asset_system/AssetModel.hpp"

namespace MFA::AssetSystem
{
    class Texture;
    struct Model;
}

namespace MFA::ResourceManager
{
    void Init();
    void Shutdown();

    // Note: No need to use Path
    [[nodiscard]]
    std::shared_ptr<AssetSystem::Model> AcquireCpuModel(
        std::string const & modelId,
        bool loadFileIfNotExists = true
    );

    [[nodiscard]]
    std::vector<std::shared_ptr<RT::GpuTexture>> AcquireGpuTextures(std::vector<std::string> const & textureIds);

    [[nodiscard]]
    std::shared_ptr<RT::GpuTexture> AcquireGpuTexture(
        std::string const & textureId,
        bool loadFileIfNotExists = true
    );

    [[nodiscard]]
    std::shared_ptr<AssetSystem::Texture> AcquireCpuTexture(
        std::string const & textureId,
        bool loadFileIfNotExists = true
    );

}

namespace MFA
{
    namespace RC = ResourceManager;
}
