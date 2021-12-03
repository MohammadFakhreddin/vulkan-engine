#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <memory>

namespace MFA
{
    namespace AssetSystem
    {
        struct Model;
    }
}

namespace MFA::ResourceManager
{
    void Init();
    void Shutdown();

    // Note: No need to use Path
    std::shared_ptr<RT::GpuModel> Acquire(char const * fileAddress, bool loadFileIfNotExists = true);

    // For shapes like sphere,cube, etc
    std::shared_ptr<RT::GpuModel> Assign(AssetSystem::Model & cpuModel, char const * name);

}


namespace MFA
{
    namespace RC = ResourceManager;
}
