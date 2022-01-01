#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <memory>

namespace MFA::AssetSystem
{
    struct Model;
}

namespace MFA::ResourceManager
{
    void Init();
    void Shutdown();

    // Note: No need to use Path
    std::shared_ptr<RT::GpuModel> AcquireForGpu(char const * nameOrFileAddress, bool loadFileIfNotExists = true);
    std::shared_ptr<AssetSystem::Model> AcquireForCpu(char const * nameOrFileAddress, bool loadFileIfNotExists = true);

    // For shapes like sphere,cube, etc
    bool Assign(std::shared_ptr<AssetSystem::Model> const & cpuModel, char const * name);

}

namespace MFA
{
    namespace RC = ResourceManager;
}
