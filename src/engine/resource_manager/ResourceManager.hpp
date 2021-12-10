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

    // Temporary function for freeing imported assets, Will be replaced with RCMGMT system in future
    //bool FreeModel(AS::Model & model);

    // Temporary function for freeing imported assets, Will be replaced with RCMGMT system in future
    //bool FreeTexture(AS::Texture & texture);

    //bool FreeShader(AS::Shader & shader);

    // Calling this function is not required because meshed does not allocate dynamic memory
    //bool FreeMesh(AS::Mesh & mesh);

}


namespace MFA
{
    namespace RC = ResourceManager;
}
