#pragma once

#include "engine/BedrockCommon.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/asset_system/AssetTexture.hpp"
#include "engine/asset_system/AssetBaseMesh.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/asset_system/AssetShader.hpp"

#include <memory>

namespace MFA::Importer {

//------------------------------Assets-------------------------------------

struct ImportTextureOptions {
    bool tryToGenerateMipmaps = true;       // Generates mipmaps for uncompressed texture
    bool preferSrgb = false;                // Not tested and not recommended
    AS::SamplerConfig * sampler = nullptr;  // For better sampling, you can specify specific options for sampler
    // TODO Usage flags
};

[[nodiscard]]
std::shared_ptr<AS::Texture> ImportUncompressedImage(
    char const * path, 
    ImportTextureOptions const & options = {}
);

[[nodiscard]]
std::shared_ptr<AS::Texture> CreateErrorTexture();

[[nodiscard]]
std::shared_ptr<AS::Texture> ImportInMemoryTexture(
    CBlob originalImagePixels,
    int32_t width,
    int32_t height,
    AS::TextureFormat format,
    uint32_t components,
    uint16_t depth = 1,
    uint16_t slices = 1,
    ImportTextureOptions const & options = {}
);

std::shared_ptr<AS::Texture> ImportKTXImage(
    char const * path,
    ImportTextureOptions const & options = {}
);

// TODO ImageArray

// TODO
[[nodiscard]]
AS::Texture ImportDDSFile(char const * path);


[[nodiscard]]
std::shared_ptr<AS::Texture> ImportImage(char const * path, ImportTextureOptions const & options = {});
/*
 * Due to lack of material support, OBJ files are not very useful (Deprecated)
 */
[[nodiscard]]
std::shared_ptr<AS::MeshBase> ImportObj(char const * path);

[[nodiscard]]
std::shared_ptr<AS::Model> ImportGLTF(char const * path);

[[nodiscard]]
std::shared_ptr<AS::Shader> ImportShaderFromHLSL(char const * path);

[[nodiscard]]
std::shared_ptr<AS::Shader> ImportShaderFromSPV(
    char const * path,
    AS::ShaderStage stage,
    char const * entryPoint
);

[[nodiscard]]
std::shared_ptr<AS::Shader> ImportShaderFromSPV(
    CBlob dataMemory,
    AS::ShaderStage const stage,
    char const * entryPoint
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
RawFile ReadRawFile(char const * path);         // We should use resource manager here


}
