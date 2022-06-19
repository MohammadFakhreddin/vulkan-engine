#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <memory>
#include <string>

#include "engine/asset_system/AssetModel.hpp"

namespace MFA
{
    class EssenceBase;
    class BasePipeline;
}

namespace MFA::AssetSystem
{
    class Texture;
    struct Model;
}

namespace MFA::ResourceManager
{
    void Init();
    void Shutdown();

    using CpuModelCallback = std::function<void(std::shared_ptr<AssetSystem::Model> const & cpuModel)>;

    // Note: No need to use Path
    void AcquireCpuModel(
        std::string const & modelId,
        CpuModelCallback const & callback,
        bool loadFromFile = true
    );

    using GpuTextureCallback = std::function<void(std::shared_ptr<RT::GpuTexture> const & gpuTexture)>;

    //// Load file if not exists is set to true by default
    //[[nodiscard]]
    //std::vector<std::shared_ptr<RT::GpuTexture>> AcquireGpuTextures(
    //    std::vector<std::string> const & textureIds,
    //    GpuTextureCallback const & callback
    //);

    void AcquireGpuTexture(
        std::string const & textureId,
        GpuTextureCallback const & callback,
        bool loadFromFile = true
    );

    using CpuTextureCallback = std::function<void(std::shared_ptr<AssetSystem::Texture> const & cpuTexture)>;

    void AcquireCpuTexture(
        std::string const & textureId,
        CpuTextureCallback const & callback,
        bool loadFromFile = true
    );

    using EssenceCallback = std::function<void(bool success)>;

    // Callback is called from main thread
    void AcquireEssence(
        std::string const & nameId,
        BasePipeline * pipeline,
        EssenceCallback const & callback,
        bool loadFromFile = true
    );

}

namespace MFA
{
    namespace RC = ResourceManager;
}
