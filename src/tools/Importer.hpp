#pragma once

#include "../engine/FoundationAsset.hpp"

namespace MFA::Importer {
//TODO:// After implementing Resource system return handle instead
//------------------------------Assets-------------------------------------

struct ImportTextureOptions {
    bool tryToGenerateMipmaps = true;       // Generates mipmaps for uncompressed texture
    bool preferSrgb = false;                // Not tested and not recommended
    AS::SamplerConfig * sampler = nullptr;  // For better sampling, you can specify specific options for sampler
    // TODO Usage flags
};

[[nodiscard]]
AS::Texture ImportUncompressedImage(
    char const * path, 
    ImportTextureOptions const & options = {}
);

[[nodiscard]]
AS::Texture CreateErrorTexture();

[[nodiscard]]
AS::Texture ImportInMemoryTexture(
    CBlob originalImagePixels,
    int32_t width,
    int32_t height,
    AS::TextureFormat format,
    uint32_t components,
    uint16_t depth = 1,
    uint16_t slices = 1,
    ImportTextureOptions const & options = {}
);

AS::Texture ImportKTXImage(
    char const * path,
    ImportTextureOptions const & options = {}
);

// TODO ImageArray

// TODO
[[nodiscard]]
AS::Texture ImportDDSFile(char const * path);


[[nodiscard]]
AS::Texture ImportImage(char const * path, ImportTextureOptions const & options = {});
/*
 * Due to lack of material support, OBJ files are not very useful (Deprecated)
 */
[[nodiscard]]
AS::Mesh ImportObj(char const * path);

[[nodiscard]]
AS::Model ImportGLTF(char const * path);

[[nodiscard]]
AS::Shader ImportShaderFromHLSL(char const * path);

[[nodiscard]]
AS::Shader ImportShaderFromSPV(
    char const * path,
    AS::ShaderStage stage,
    char const * entryPoint
);

[[nodiscard]]
AS::Shader ImportShaderFromSPV(
    CBlob dataMemory,
    AS::ShaderStage const stage,
    char const * entryPoint
);

// Temporary function for freeing imported assets, Will be replaced with RCMGMT system in future
bool FreeModel(AS::Model & model);

// Temporary function for freeing imported assets, Will be replaced with RCMGMT system in future
bool FreeTexture(AS::Texture & texture);

bool FreeShader(AS::Shader & shader);

// Calling this function is not required because meshed does not allocate dynamic memory
bool FreeMesh(AS::Mesh & mesh);

//------------------------------RawFile------------------------------------

struct RawFile {
    Blob data;
    [[nodiscard]]
    bool valid() const {
        return data.ptr != nullptr;
    }
};

[[nodiscard]]
RawFile ReadRawFile(char const * path);

bool FreeRawFile (RawFile * rawFile);

}
