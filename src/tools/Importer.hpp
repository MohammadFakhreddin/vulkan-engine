#ifndef IMPORTER_NAMESPACE
#define IMPORTER_NAMESPACE

#include "../engine/FoundationAsset.hpp"

namespace MFA::Importer {
//TODO:// After implementing Resource system return handle instead

//------------------------------Assets-------------------------------------

struct ImportUncompressedImageOptions {
    bool generate_mipmaps = true;
    bool prefer_srgb = false;       // Not tested and not recommended
    // TODO Usage flags
};

[[nodiscard]]
Asset::TextureAsset ImportUncompressedImage(
    char const * path, 
    ImportUncompressedImageOptions options
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


#endif