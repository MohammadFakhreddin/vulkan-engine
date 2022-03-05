#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <memory>
#include <string>

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
    std::shared_ptr<RT::GpuModel> AcquireGpuModel(std::string const & nameOrFileAddress, bool loadFileIfNotExists = true);
    std::shared_ptr<AssetSystem::Model> AcquireCpuModel(std::string const & nameOrFileAddress, bool loadFileIfNotExists = true);

    // TODO Support for acquiring mesh
    std::shared_ptr<RT::GpuTexture> AcquireGpuTexture(std::string const & nameOrFileAddress, bool loadFileIfNotExists = true);
    std::shared_ptr<AssetSystem::Texture> AcquireCpuTexture(std::string const & nameOrFileAddress, bool loadFileIfNotExists = true);

    // For shapes like sphere,cube, etc
    //bool Assign(std::shared_ptr<AssetSystem::Model> const & cpuModel, std::string const & name);

}

namespace MFA
{
    namespace RC = ResourceManager;
}
