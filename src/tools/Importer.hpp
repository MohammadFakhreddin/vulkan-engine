#pragma once

#include <vector>

#include "../engine/FoundationAsset.hpp"

namespace MFA::Importer {
//TODO:// After implementing Resource system return handle instead

//------------------------------Assets-------------------------------------

struct ImportTextureOptions {
    bool generate_mipmaps = true;
    bool prefer_srgb = false;       // Not tested and not recommended
    AssetSystem::TextureHeader::Sampler * sampler = nullptr;
    // TODO Usage flags
};

[[nodiscard]]
AssetSystem::TextureAsset ImportUncompressedImage(
    char const * path, 
    ImportTextureOptions const & options = {}
);

[[nodiscard]]
AssetSystem::TextureAsset ImportInMemoryTexture(
    CBlob pixels,
    I32 width,
    I32 height,
    AssetSystem::TextureFormat format,
    U32 components,
    U16 depth = 1,
    I32 slices = 1,
    ImportTextureOptions const & options = {}
);

// TODO ImageArray

// TODO
[[nodiscard]]
AssetSystem::TextureAsset ImportDDSFile(char const * path);
/*
 * Due to lack of material support, OBJ files are not very useful (Deprecated)
 */
[[nodiscard]]
AssetSystem::MeshAsset ImportObj(char const * path);

[[nodiscard]]
AssetSystem::ModelAsset ImportMeshGLTF(char const * path);

[[nodiscard]]
AssetSystem::ShaderAsset ImportShaderFromHLSL(char const * path);

[[nodiscard]]
AssetSystem::ShaderAsset ImportShaderFromSPV(
    char const * path,
    AssetSystem::ShaderStage stage,
    char const * entry_point
);

[[nodiscard]]
AssetSystem::ShaderAsset ImportShaderFromSPV(
    CBlob data_memory,
    AssetSystem::ShaderStage const stage,
    char const * entry_point
);

// Temporary function for freeing imported assets, Will be replaced with RCMGMT system in future
bool FreeModel(AssetSystem::ModelAsset * model);

// Temporary function for freeing imported assets, Will be replaced with RCMGMT system in future
bool FreeAsset(AssetSystem::GenericAsset * asset);

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