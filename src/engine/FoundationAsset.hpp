#ifndef ASSET_NAMESPACE
#define ASSET_NAMESPACE

#include "BedrockAssert.hpp"
#include "BedrockCommon.hpp"
#include "BedrockMath.hpp"

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
    UNCOMPRESSED_UNORM_R8G8B8A8_SRGB    = 5,
    
    BC7_UNorm_Linear_RGB        = 6,
    BC7_UNorm_Linear_RGBA       = 7,
    BC7_UNorm_sRGB_RGB          = 8,
    BC7_UNorm_sRGB_RGBA         = 9,

    BC6H_UFloat_Linear_RGB      = 10,
    BC6H_SFloat_Linear_RGB      = 11,

    BC5_UNorm_Linear_RG         = 12,
    BC5_SNorm_Linear_RG         = 13,

    BC4_UNorm_Linear_R          = 14,
    BC4_SNorm_Linear_R          = 15,

    Count
};

inline constexpr struct {
    Format texture_format;
    uint8_t compression;                            // 0: uncompressed, 1: basis?, ...
    uint8_t component_count;                        // 1..4
    uint8_t component_format;                       // 0: UNorm, 1: SNorm, 2: UInt, 3: SInt, 4: UFloat, 5: SFloat
    uint8_t color_space;                            // 0: Linear, 1: sRGB
    uint8_t bits_r, bits_g, bits_b, bits_a;         // each 0..32
    uint8_t bits_total;                             // 1..128
} FormatTable [] = {
    {Format::INVALID                                 , 0, 0, 0, 0, 0, 0, 0, 0,  0},

    {Format::UNCOMPRESSED_UNORM_R8_LINEAR            , 0, 1, 0, 0, 8, 0, 0, 0,  8},
    {Format::UNCOMPRESSED_UNORM_R8G8_LINEAR          , 0, 2, 0, 0, 8, 8, 0, 0, 16},
    {Format::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR      , 0, 4, 0, 0, 8, 8, 8, 8, 32},
    {Format::UNCOMPRESSED_UNORM_R8_SRGB              , 0, 1, 0, 1, 8, 0, 0, 0,  8},
    {Format::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB        , 0, 4, 0, 1, 8, 8, 8, 8, 32},

    {Format::BC7_UNorm_Linear_RGB                    , 7, 3, 0, 0, 8, 8, 8, 0,  8},
    {Format::BC7_UNorm_Linear_RGBA                   , 7, 4, 0, 0, 8, 8, 8, 8,  8},
    {Format::BC7_UNorm_sRGB_RGB                      , 7, 3, 0, 1, 8, 8, 8, 0,  8},
    {Format::BC7_UNorm_sRGB_RGBA                     , 7, 4, 0, 1, 8, 8, 8, 8,  8},

    {Format::BC6H_UFloat_Linear_RGB                  , 6, 3, 4, 0, 16, 16, 16, 0, 8},
    {Format::BC6H_SFloat_Linear_RGB                  , 6, 3, 5, 0, 16, 16, 16, 0, 8},

    {Format::BC5_UNorm_Linear_RG                     , 5, 2, 0, 0, 8, 8, 0, 0, 8},
    {Format::BC5_SNorm_Linear_RG                     , 5, 2, 0, 1, 8, 8, 0, 0, 8},

    {Format::BC4_UNorm_Linear_R                      , 5, 2, 0, 0, 8, 8, 0, 0, 16},
    {Format::BC4_SNorm_Linear_R                      , 5, 2, 0, 1, 8, 8, 0, 0, 16},

};
static_assert(ArrayCount(FormatTable) == static_cast<unsigned>(Format::Count));


#pragma pack(push)
#pragma warning (push)
#pragma warning (disable: 4200)         // Non-standard extension used: zero-sized array in struct
struct Header {
    struct Dimensions {
        U32 width = 0;
        U32 height = 0;
        U16 depth = 0;
    };
    //static_assert(10 == sizeof(Dimensions));
    struct MipmapInfo {
        U32 offset {};
        U32 size {};
        Dimensions dims {};
    };
    //static_assert(18 == sizeof(MipmapInfo));
    // TODO Handle alignment (Size and reserved if needed) for write operation to .asset file
    Format          format = Format::INVALID;
    U16             slices = 0;
    U8              mip_count = 0;
    U16             depth = 0;
    MipmapInfo      mipmap_infos[];

    [[nodiscard]]
    size_t mip_offset_bytes (uint8_t const mip_level, uint8_t const slice_index = 0) const {
        size_t ret = 0;
        if(mip_level < mip_count && slice_index < slices) {
            ret = mipmap_infos[mip_level].offset + slice_index * mipmap_infos[mip_level].size;
        }
        return ret;
    }

    static U8 ComputeMipCount(Dimensions const dimensions) {
        U32 const max_dimension = Math::Max(
            dimensions.width, 
            Math::Max(
                dimensions.height, 
                static_cast<uint32_t>(dimensions.depth)
            )
        );
        for (U8 i = 0; i < 32; ++i)
            if ((1ULL << i) >= max_dimension)
                return 1 + i;
        return 33;
    }

    /*
     * This function result is only correct for uncompressed data
     */
    [[nodiscard]]
    static size_t MipSizeBytes (
        Format format,
        uint16_t const slices,
        Dimensions const mip_level_dims
    )
    {
        auto const & d = mip_level_dims;
        size_t const p = FormatTable[static_cast<unsigned>(format)].bits_total / 8;
        return p * slices * d.width * d.height * d.depth;
    }

    // TODO Consider moving this function to util_image
    /*
     * This function result is only correct for uncompressed data
     */
    // NOTE: 0 is the *smallest* mipmap level, and "mip_count - 1" is the *largest*.
    [[nodiscard]]
    static Dimensions MipDimensions (
        uint8_t const mip_level,
        uint8_t const mip_count,
        Dimensions const original_image_dims
    )
    {
        Dimensions ret = {};
        if (mip_level < mip_count) {
            uint32_t const pow = mip_count - 1 - mip_level;
            uint32_t const add = (1 << pow) - 1;
            ret.width = (original_image_dims.width + add) >> pow;
            ret.height = (original_image_dims.height + add) >> pow;
            ret.depth = static_cast<uint16_t>((original_image_dims.depth + add) >> pow);
        }
        return ret;
    }

    /*
    * This function result is only correct for uncompressed data
    * Returns space required for both mipmaps and TextureHeader
    */
    [[nodiscard]]
    static size_t CalculateUncompressedTextureRequiredDataSize(
        Format const format,
        uint16_t const slices,
        Dimensions const & dims,
        uint8_t const mip_count
    ) {
        size_t ret = 0;
        for (uint8_t mip_level = 0; mip_level < mip_count; mip_level++) {
            ret += MipSizeBytes(
                format, 
                slices, 
                MipDimensions(mip_level, mip_count, dims)
            );
        }
        return ret;
    }

    [[nodiscard]]
    static size_t Size(U8 const mip_count) {
        return sizeof(Header) + (mip_count * sizeof(MipmapInfo));
    }
    [[nodiscard]]
    size_t size() {
        return Size(mip_count);
    }
    [[nodiscard]]
    bool is_valid() const {
        // TODO
        return true;
    }
};
#pragma warning(pop)
#pragma pack(pop)

}

using TextureFormat = Texture::Format;
using TextureHeader = Texture::Header;

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
#pragma warning (push)
#pragma warning (disable: 4200)         // Non-standard extension used: zero-sized array in struct
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
#pragma warning (pop)
}}

using MeshHeader = Mesh::Header;
using MeshVertices = Mesh::Data::Vertices;
using MeshIndices = Mesh::Data::Indices;

//--------------------------------ShaderHeader--------------------------------------
namespace Shader {

enum class Stage {
    Invalid,
    Vertex,
    Fragment
};

struct Header {
    std::string entry_point {};     // Ex: main
    Stage stage = Stage::Invalid;
    [[nodiscard]]
    static size_t Size() {
        return sizeof(Header);
    }
    [[nodiscard]]
    bool is_valid() const {
        return false == entry_point.empty() && Stage::Invalid != stage;
    }
};  

}

using ShaderHeader = Shader::Header;
using ShaderStage = Shader::Stage;

//--------------------------------MaterialHeader-------------------------------------

struct MaterialHeader {}; // TODO

//---------------------------------GenericAsset----------------------------------

class GenericAsset {
public:
    GenericAsset() : m_asset({}) {}
    explicit GenericAsset(Blob const asset_) : m_asset(asset_) {}
    [[nodiscard]]
    CBlob header_blob() const {
        return CBlob {m_asset.ptr, compute_header_size()};
    }
    [[nodiscard]]
    virtual size_t compute_header_size() const = 0;
    [[nodiscard]]
    CBlob data() const {
        auto const header_size = compute_header_size();
        return CBlob {
            m_asset.ptr + header_size,
            m_asset.len - header_size
        };
    }
    [[nodiscard]]
    virtual bool valid() const = 0;
    [[nodiscard]]
    Blob asset() const {return m_asset;}
    void set_asset(Blob const asset_) {m_asset = asset_;}
private:
    Blob m_asset;
};

//---------------------------------TextureAsset-----------------------------------

class TextureAsset : public GenericAsset {
public:
    TextureAsset() = default;
    explicit TextureAsset(Blob const asset_) : GenericAsset(asset_) {}
    [[nodiscard]]
    size_t compute_header_size() const override {
        return Texture::Header::Size(asset().as<Texture::Header>()->mip_count);
    }
    [[nodiscard]]
    CBlob mip_data (uint8_t const mip_level) const {
        auto const * texture_header = asset().as<Texture::Header>();
        return {
            asset().ptr + texture_header->mipmap_infos[mip_level].offset,
            texture_header->mipmap_infos[mip_level].size
        };
    }
    [[nodiscard]]
    CBlob slice_data (uint8_t const mip_level, uint16_t const slice) const {
        MFA_ASSERT(slice < header_object()->slices);
        MFA_ASSERT(mip_level < header_object()->mip_count);
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
    [[nodiscard]]
    bool valid() const override {
        return MFA_PTR_VALID(asset().ptr) && header_object()->is_valid();
    }
};

//----------------------------------ShaderAsset------------------------------------

class ShaderAsset : public GenericAsset {
public:
    ShaderAsset() = default;
    explicit  ShaderAsset(Blob const asset_) : GenericAsset(asset_) {};
    [[nodiscard]]
    size_t compute_header_size() const override {return sizeof(ShaderHeader);}
    [[nodiscard]]
    ShaderHeader const * header_object() const {
        return header_blob().as<ShaderHeader>();
    }
    [[nodiscard]]
    bool valid() const override {
        return MFA_PTR_VALID(asset().ptr) && header_object()->is_valid();
    }
};

//-----------------------------------MeshAsset------------------------------------

class MeshAsset : public GenericAsset {
public:
    MeshAsset() = default;
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
        return reinterpret_cast<MeshVertices const *>(asset().ptr + header->vertices_offset);
    }
    [[nodiscard]]
    MeshIndices const * indices() const {
        auto const * header = header_object();
        return reinterpret_cast<MeshIndices const *>(asset().ptr + header->indices_offset);
    }
    [[nodiscard]]
    bool valid() const override {
        return MFA_PTR_VALID(asset().ptr) && header_object()->is_valid();
    }
};

//---------------------------------MaterialAsset----------------------------------

class MaterialAsset : public GenericAsset {
    // TODO
};

};  // MFA::Asset

#endif