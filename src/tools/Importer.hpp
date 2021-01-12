#pragma once

#include "../engine/FoundationAsset.hpp"

namespace MFA::Importer {
//TODO:// After implementing Resource system return handle instead

//------------------------------Assets-------------------------------------

struct ImportTextureOptions {
    bool generate_mipmaps = true;
    bool prefer_srgb = false;       // Not tested and not recommended
    // TODO Usage flags
};

[[nodiscard]]
Asset::TextureAsset ImportUncompressedImage(
    char const * path, 
    ImportTextureOptions const & options = {}
);

[[nodiscard]]
Asset::TextureAsset ImportInMemoryTexture(
    CBlob pixels,
    I32 width,
    I32 height,
    Asset::TextureFormat format,
    U32 components,
    U16 depth = 1,
    I32 slices = 1,
    ImportTextureOptions const & options = {}
);

// TODO ImageArray

// TODO
[[nodiscard]]
Asset::TextureAsset ImportDDSFile(char const * path);

[[nodiscard]]
Asset::MeshAsset ImportObj(char const * path);

[[nodiscard]]
Asset::MeshAsset ImportGLTF(char const * path);

[[nodiscard]]
Asset::ShaderAsset ImportShaderFromHLSL(char const * path);

[[nodiscard]]
Asset::ShaderAsset ImportShaderFromSPV(
    char const * path,
    Asset::ShaderStage stage,
    char const * entry_point
);

[[nodiscard]]
Asset::ShaderAsset ImportShaderFromSPV(
    CBlob data_memory,
    Asset::ShaderStage const stage,
    char const * entry_point
);

// Temporary function for freeing imported assets
bool FreeAsset(Asset::GenericAsset * asset);

//------------------------------RawFile------------------------------------

struct RawFile {
    Blob data;
    [[nodiscard]]
    bool valid() const {
        return MFA_PTR_VALID(data.ptr);
    }
};

[[nodiscard]]
RawFile ReadRawFile(char const * path);

bool FreeRawFile (RawFile * raw_file);

}