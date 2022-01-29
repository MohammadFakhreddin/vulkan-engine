#pragma once

#include "engine/asset_system/AssetTypes.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/BedrockMemory.hpp"

#include <memory>

namespace MFA
{
    struct SmartBlob;
}

namespace MFA::AssetSystem
{
    class Texture;
    class MeshBase;
    struct Model;
    class Shader;
}

namespace MFA::Importer {

//------------------------------Assets-------------------------------------

struct ImportTextureOptions {
    //I think I will remove this op
    bool tryToGenerateMipmaps = false;      // Generates mipmaps for uncompressed texture
    bool preferSrgb = false;                // Not tested and not recommended
    AssetSystem::SamplerConfig * sampler = nullptr;  // For better sampling, you can specify specific options for sampler
    // TODO Usage flags
};

[[nodiscard]]
std::shared_ptr<AssetSystem::Texture> ImportUncompressedImage(
    std::string const & path, 
    ImportTextureOptions const & options = {}
);

[[nodiscard]]
std::shared_ptr<AssetSystem::Texture> CreateErrorTexture();

[[nodiscard]]
std::shared_ptr<AssetSystem::Texture> ImportInMemoryTexture(
    CBlob originalImagePixels,
    int32_t width,
    int32_t height,
    AssetSystem::TextureFormat format,
    uint32_t components,
    uint16_t depth = 1,
    uint16_t slices = 1,
    ImportTextureOptions const & options = {}
);

std::shared_ptr<AssetSystem::Texture> ImportKTXImage(
    std::string const & path,
    ImportTextureOptions const & options = {}
);

// TODO ImageArray

// TODO
[[nodiscard]]
AssetSystem::Texture ImportDDSFile(std::string const & path);


[[nodiscard]]
std::shared_ptr<AssetSystem::Texture> ImportImage(std::string const & path, ImportTextureOptions const & options = {});
/*
 * Due to lack of material support, OBJ files are not very useful (Deprecated)
 */
[[nodiscard]]
std::shared_ptr<AssetSystem::MeshBase> ImportObj(std::string const & path);

[[nodiscard]]
std::shared_ptr<AssetSystem::Model> ImportGLTF(std::string const & path);

[[nodiscard]]
std::shared_ptr<AssetSystem::Shader> ImportShaderFromHLSL(std::string const & path);

[[nodiscard]]
std::shared_ptr<AssetSystem::Shader> ImportShaderFromSPV(
    std::string const & path,
    AssetSystem::ShaderStage stage,
    std::string const & entryPoint
);

[[nodiscard]]
std::shared_ptr<AssetSystem::Shader> ImportShaderFromSPV(
    CBlob dataMemory,
    AssetSystem::ShaderStage stage,
    std::string const & entryPoint
);

//------------------------------RawFile------------------------------------

struct RawFile {
    std::shared_ptr<SmartBlob> data = nullptr;
    [[nodiscard]]
    bool valid() const {
        return data->memory.ptr != nullptr;
    }
};

[[nodiscard]]
RawFile ReadRawFile(std::string const & path);         // We should use resource manager here


}
