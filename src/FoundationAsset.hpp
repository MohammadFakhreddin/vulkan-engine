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

//----------------------------------Texture------------------------------
namespace Texture {

enum class Format {
    INVALID     = 0,
    UNCOMPRESSED_UNORM_R8_LINEAR        = 1,
    UNCOMPRESSED_UNORM_R8G8_LINEAR      = 2,
    UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR  = 3,
    UNCOMPRESSED_UNORM_R8_SRGB          = 4,
    UNCOMPRESSED_UNORM_R8G8B8A8_SRGB    = 5
};

#pragma pack(push)
struct Header {
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
    Format          format = Format::INVALID;
    U16             slices = 0;
    U8              mip_count = 0;
    MipmapInfo      mipmap_infos[];

    [[nodiscard]]
    static size_t Size(U8 const mip_count) {
        return sizeof(Header) + (mip_count * sizeof(MipmapInfo));
    }
    [[nodiscard]]
    bool is_valid() const {
        // TODO
        return true;
    }
};
#pragma pack(pop)

}

using TextureHeader = Texture::Header;
using TextureFormat = Texture::Format;

//---------------------------------MeshHeader-------------------------------------
namespace Mesh {

struct Header {
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t vertices_offset;       // From start of asset
    uint32_t indices_offset;        // From start of asset
    [[nodiscard]]
    static size_t Size() {
        return sizeof(Header);
    }
    [[nodiscard]]
    bool is_valid() const {
        // TODO
        return true;
    }
};

namespace Data {

struct Vertices {
    using Position = float[3];
    using Normal = float[3];
    using UV = float[2];
    using Color = U8[3];
    struct Vertex {
        Position position;
        Normal normal;
        UV uv;
        Color color;
    };
    Vertex vertices[];
};

struct Indices {
    U32 indices[];
};

}}

using MeshHeader = Mesh::Header;
using MeshVertices = Mesh::Data::Vertices;
using MeshIndices = Mesh::Data::Indices;

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
    explicit TextureAsset(Blob const asset_) : GenericAsset(asset_) {}
    [[nodiscard]]
    size_t compute_header_size() const override {
        return Texture::Header::Size(m_asset.as<Texture::Header>()->mip_count);
    }
    [[nodiscard]]
    CBlob mip_data (uint8_t const mip_level) const {
        auto const * texture_header = m_asset.as<Texture::Header>();
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
    Texture::Header const * header_object() const {
        return header_blob().as<Texture::Header>();
    }
};

//----------------------------------ShaderAsset------------------------------------

class ShaderAsset : public GenericAsset {
    // TODO
};

//-----------------------------------MeshAsset------------------------------------

class MeshAsset : public GenericAsset {
public:
    explicit MeshAsset(Blob const asset_) : GenericAsset(asset_) {}
    [[nodiscard]]
    size_t compute_header_size() const override {
        return Mesh::Header::Size();
    }
    [[nodiscard]]
    MeshHeader const * header_object() const {
        return header_blob().as<Mesh::Header>();
    }
    [[nodiscard]]
    MeshVertices const * vertices() const {
        auto const * header = header_object();
        return reinterpret_cast<MeshVertices const *>(m_asset.ptr + header->vertices_offset);
    }
    [[nodiscard]]
    MeshIndices const * indices() const {
        auto const * header = header_object();
        return reinterpret_cast<MeshIndices const *>(m_asset.ptr + header->indices_offset);
    }
};

//---------------------------------MaterialAsset----------------------------------

class MaterialAsset : public GenericAsset {
    // TODO
};

};  // MFA::Asset

#endif