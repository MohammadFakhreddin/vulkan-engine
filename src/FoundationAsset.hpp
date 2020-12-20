#ifndef ASSET_HEADER
#define ASSET_HEADER

#include "BedrockAssert.hpp"
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

//----------------------------------TextureHeader------------------------------

enum class TextureFormat {
    INVALID     = 0,
    UNCOMPRESSED_UNORM_R8_LINEAR        = 1,
    UNCOMPRESSED_UNORM_R8G8_LINEAR      = 2,
    UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR  = 3,
    UNCOMPRESSED_UNORM_R8_SRGB          = 4,
    UNCOMPRESSED_UNORM_R8G8B8A8_SRGB    = 5
};
#pragma pack(push)
struct TextureHeader {
    struct Dimensions {
        U32 width = 0;
        U32 height = 0;
        U16 depth = 0;
    };
    static_assert(10 == sizeof(Dimensions));
    struct MipmapInfo {
        U32 offset {};
        U32 size {};
        Dimensions dims {};
    };
    static_assert(18 == sizeof(MipmapInfo));
    // TODO Handle alignment (Size and reserved if needed) for write operation to .asset file
    TextureFormat   format = TextureFormat::INVALID;
    U16             slices = 0;
    U8              mip_count = 0;
    MipmapInfo      mipmap_infos[];

};
#pragma pack(pop)

[[nodiscard]]
inline size_t TextureHeaderSize(U8 const mip_count) {
    return sizeof(TextureHeader) + (mip_count * sizeof(TextureHeader::MipmapInfo));
};

[[nodiscard]]
inline bool IsTextureValue(TextureHeader const * texture_header) {
    // TODO
    return true;
}

//---------------------------------MeshHeader-------------------------------------

struct MeshHeader {
    // TODO Rename Node to something else
    struct Node {
        U32 position_index;
        U32 normal_index;
        U32 texture_coordinate_index;
    };
    U32 position_array_count;
    U32 normal_array_count;
    U32 texture_coordinate_array_count;
    U16 nodes_count;
    Node nodes[];
}; // TODO

[[nodiscard]]
inline size_t MeshHeaderSize(U16 const nodes_count) {
    return sizeof(MeshHeader) + (nodes_count * sizeof(MeshHeader::Node));
}

//--------------------------------ShaderHeader--------------------------------------

struct ShaderHeader {};  // TODO

//--------------------------------MaterialHeader-------------------------------------

struct MaterialHeader {}; // TODO

//---------------------------------GenericAsset----------------------------------

class GenericAsset {
public:
    explicit GenericAsset(Blob const asset_) : m_asset(asset_) {}
    [[nodiscard]]
    CBlob header_blob() const {
        return CBlob {m_asset.ptr, compute_header_size()};
    }
    [[nodiscard]]
    virtual size_t compute_header_size() const;
    [[nodiscard]]
    CBlob data() const {
        auto const header_size = compute_header_size();
        return CBlob {
            m_asset.ptr + header_size,
            m_asset.len - header_size
        };
    }
protected:
    Blob m_asset;
};

//---------------------------------TextureAsset-----------------------------------

class TextureAsset : public GenericAsset {
public:
    [[nodiscard]]
    size_t compute_header_size() const override {
        return TextureHeaderSize(m_asset.as<TextureHeader>()->mip_count);
    }
    [[nodiscard]]
    CBlob mip_data (uint8_t const mip_level) const {
        auto const * texture_header = m_asset.as<TextureHeader>();
        return {
            m_asset.ptr + texture_header->mipmap_infos[mip_level].offset,
            texture_header->mipmap_infos[mip_level].size
        };
    }
    [[nodiscard]]
    CBlob slice_data (uint8_t const mip_level, uint16_t const slice) const {
        MFA_ASSERT(slice < header().slices);
        MFA_ASSERT(mip_level < header().mip_count);
        auto const blob = mip_data(mip_level);
        return {
            blob.ptr + slice * blob.len,
            blob.len
        };
    }
    [[nodiscard]]
    TextureHeader const * header_object() const {
        return header_blob().as<TextureHeader>();
    }
};

//----------------------------------ShaderAsset------------------------------------

class ShaderAsset : public GenericAsset {
    // TODO
};

//-----------------------------------MeshAsset------------------------------------

class MeshAsset : public GenericAsset {
    // TODO
};

//---------------------------------MaterialAsset----------------------------------

class MaterialAsset : public GenericAsset {
    // TODO
};

};  // MFA::Asset

#endif