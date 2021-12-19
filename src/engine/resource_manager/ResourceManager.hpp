#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/FoundationAssetFWD.hpp"

#include <memory>

namespace MFA::ResourceManager
{
    void Init();
    void Shutdown();

    // Note: No need to use Path
    std::shared_ptr<RT::GpuModel> Acquire(char const * fileAddress, bool loadFileIfNotExists = true);

    // For shapes like sphere,cube, etc
    std::shared_ptr<RT::GpuModel> Assign(std::shared_ptr<AS::Model> cpuModel, char const * name);

}


namespace MFA
{
    namespace RC = ResourceManager;
}
