#ifndef ASSET_HEADER
#define ASSET_HEADER

#include "BedrockCommon.hpp"

namespace MFA::Asset {
// TODO We need hash functionality as well
enum class AssetType {
    INVALID     = 0,
    Texture     = 1,
    Mesh        = 2,
    Shader      = 3,
    Material    = 4
};

//-------------------------------------Texture------------------------------

enum class TextureFormat {
    INVALID     = 0,
    UNCOMPRESSED_UNORM_R8_LINEAR        = 1,
    UNCOMPRESSED_UNORM_R8G8_LINEAR      = 2,
    UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR  = 3,
    UNCOMPRESSED_UNORM_R8_SRGB          = 4,
    UNCOMPRESSED_UNORM_R8G8B8A8_SRGB    = 5
};

struct TextureHeader {
    struct Dimensions {
        U32 width = 0;
        U32 height = 0;
        U16 depth = 0;
    };
    static_assert(10 == sizeof(Dimensions));
    struct MipmapInfo {
        U32 offset;
        U32 size;
        Dimensions dims;
    };
    static_assert(18 == sizeof(MipmapInfo));
    // TODO Handle alignment (Size and reserved if needed) for write operation to .asset file
    TextureFormat   format;
    U16             slices = 0;
    U8              mip_count = 0;
    MipmapInfo      mipmap_infos[];

};

[[nodiscard]]
inline size_t TextureHeaderSize(U8 const mip_count) {
    return sizeof(TextureHeader) + (mip_count * sizeof(TextureHeader::MipmapInfo));
};

[[nodiscard]]
inline bool IsTextureValue(TextureHeader const * texture_header) {
    // TODO
    return true;
}

//------------------------------------Mesh-------------------------------------

//-----------------------------------Shader------------------------------------

//-----------------------------------Texture-----------------------------------

};  // MFA::Asset

#endif