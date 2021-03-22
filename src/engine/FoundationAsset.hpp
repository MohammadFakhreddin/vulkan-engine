#pragma once

#include "BedrockMatrix.hpp"
#include "BedrockCommon.hpp"
#include "BedrockMath.hpp"

#include <vector>

namespace MFA::AssetSystem {

enum class AssetType {
    INVALID     = 0,
    Texture     = 1,
    Mesh        = 2,
    Shader      = 3,
    Material    = 4
};

//----------------------------------Texture------------------------------
class Texture {
public:
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

    struct Dimensions {
        U32 width = 0;
        U32 height = 0;
        U16 depth = 0;
    };

    struct MipmapInfo {
        U32 offset {};
        U32 size {};
        Dimensions dims {};
    };

    struct Sampler {
        bool isValid = false;
        enum class SampleMode {
            Linear,
            Nearest
        };
        SampleMode sampleMode = SampleMode::Linear;
        U32 magFilter = 0;
        U32 minFilter = 0;
        U32 wrapS = 0;
        U32 wrapT = 0;
    };

private:
    struct InternalFormatTableType {
        Format texture_format;
        uint8_t compression;                            // 0: uncompressed, 1: basis?, ...
        uint8_t component_count;                        // 1..4
        uint8_t component_format;                       // 0: UNorm, 1: SNorm, 2: UInt, 3: SInt, 4: UFloat, 5: SFloat
        uint8_t color_space;                            // 0: Linear, 1: sRGB
        uint8_t bits_r, bits_g, bits_b, bits_a;         // each 0..32
        uint8_t bits_total;                             // 1..128
    };
    static constexpr InternalFormatTableType FormatTable [] = {
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
public:

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

    explicit Texture(
        const Format format,
        const U16 slices,
        const U8 mipCount,
        const U16 depth,
        Sampler const & sampler,
        Blob const & data
    ) {
        MFA_ASSERT(format != Format::INVALID);
        mFormat = format;
        MFA_ASSERT(slices > 0);
        mSlices = slices;
        MFA_ASSERT(mipCount > 0);
        mMipCount = mipCount;
        MFA_ASSERT(depth > 0);
        mDepth = depth;
        MFA_ASSERT(sampler.isValid);
        mSampler = sampler;
        mData = data;
    }

    [[nodiscard]]
    size_t mipOffsetInBytes (uint8_t const mip_level, uint8_t const slice_index = 0) const {
        size_t ret = 0;
        if(mip_level < mMipCount && slice_index < mSlices) {
            ret = mMipmapInfos[mip_level].offset + slice_index * mMipmapInfos[mip_level].size;
        }
        return ret;
    }

    [[nodiscard]]
    bool isValid() const {
        return
            mFormat != Format::INVALID && 
            mSlices > 0 && 
            mMipCount > 0 && 
            mMipmapInfos.empty() == false && 
            mData.ptr != nullptr && 
            mData.len > 0;
    }

private:
    Format mFormat = Format::INVALID;
    U16 mSlices = 0;
    U8 mMipCount = 0;
    U16 mDepth = 0;
    Sampler mSampler {};
    std::vector<MipmapInfo> mMipmapInfos {};
    Blob mData {};
};

using TextureFormat = Texture::Format;
using TextureSampler = Texture::Sampler;

//---------------------------------MeshAsset-------------------------------------
class Mesh {
// TODO Implement serialize and deserialize
    using SubMeshIndexType = U32;

    using Position = float[3];
    using Normal = float[3];
    using UV = float[2];
    using Color = U8[3];
    using Tangent = float[4]; // XYZW vertex tangents where the w component is a sign value (-1 or +1) indicating handedness of the tangent basis. I need to change XYZ order if handness is different from mine

    using IndexType = U32;
public:
    struct Vertex {
        Position position;
        UV base_color_uv;
        UV normal_map_uv;
        UV metallic_uv;
        UV roughness_uv;
        UV emission_uv;
        Color color;
        Normal normal_value;
        Tangent tangent_value;
    };
    
    struct SubMesh {
        U32 vertex_count = 0;
        U32 index_count = 0;
        U64 vertices_offset = 0;                // From start of asset
        U64 indices_offset = 0;
        U32 indices_starting_index = 0;         // From start of asset
        I16 base_color_texture_index = 0;
        I16 metallic_roughness_texture_index = 0;
        I16 metallic_texture_index = 0;
        I16 roughness_texture_index = 0;
        I16 normal_texture_index = 0;
        I16 emissive_texture_index = 0;
        float base_color_factor[4] {};
        float metallic_factor = 0;              // Metallic color is stored inside blue
        float roughness_factor = 0;             // Roughness color is stored inside green
        float emissive_factor[3] {};
        bool has_normal_buffer = false;
        bool has_normal_texture = false;
        bool has_tangent_buffer = false;
        bool has_base_color_texture = false;
        bool has_emissive_texture = false;
        bool has_combined_metallic_roughness_texture = false;
        bool has_metallic_texture = false;
        bool has_roughness_texture = false;
    };
    
    struct Node {
        Node * parent {};
        std::vector<Node> children {};
        Matrix4X4Float transformMatrix {};
        SubMeshIndexType subMeshIndex;
    };

    void insertVertex(Vertex const & vertex) {
        mVertices.emplace_back(vertex);
    }

    void insertIndex(IndexType const & index) {
        mIndices.emplace_back(index);
    }

    void insertMesh(SubMesh const & subMesh) {
        mSubMeshes.emplace_back(subMesh);
    }

    void insertNode(Node const & node) {
        mNodes.emplace_back(node);
    }

    [[nodiscard]]
    bool isValid() const {
        // TODO
        return mVertices.empty() == false && 
            mIndices.empty() == false && 
            mSubMeshes.empty() == false && 
            mNodes.empty() == false;
    }

    [[nodiscard]]
    Vertex const * GetVerticesData() const noexcept {
        return mVertices.data();
    }

    [[nodiscard]]
    U32 GetVerticesCount() const noexcept {
        return static_cast<U32>(mVertices.size());
    }

    [[nodiscard]]
    IndexType const * GetIndicesData() const noexcept {
        return mIndices.data();
    }

    [[nodiscard]]
    U32 GetIndicesCount() const noexcept {
        return static_cast<U32>(mIndices.size());
    }

    [[nodiscard]]
    SubMesh const * GetSubMeshData() const noexcept {
        return mSubMeshes.data();
    }

    [[nodiscard]]
    U32 GetSubMeshSize() const noexcept {
        return static_cast<U32>(mSubMeshes.size());
    }

    [[nodiscard]]
    Node const * GetNodeData() const noexcept {
        return mNodes.data();
    }

    [[nodiscard]]
    U32 GetNodeSize() const noexcept {
        return static_cast<U32>(mNodes.size());
    }

private:
    std::vector<Vertex> mVertices {};
    std::vector<IndexType> mIndices {};
    std::vector<SubMesh> mSubMeshes {};
    std::vector<Node> mNodes {};

};

//--------------------------------ShaderAsset--------------------------------------
class Shader {
public:
    enum class Stage {
        Invalid,
        Vertex,
        Fragment
    };
    [[nodiscard]]
    bool is_valid() const {
        return strlen(entry_point) > 0 && Stage::Invalid != stage;
    }
private:
    static constexpr U8 EntryPointLength = 30;
    char entry_point [EntryPointLength] {};     // Ex: main
    Stage stage = Stage::Invalid;
};

using ShaderStage = Shader::Stage;

/*//---------------------------------GenericAsset----------------------------------

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
    CBlob data_cblob() const {
        auto const & blob = data_blob();
        return CBlob {
            blob.ptr,
            blob.len
        };
    }
    [[nodiscard]]
    Blob data_blob() const {
        auto const header_size = compute_header_size();
        return Blob {
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
        auto const * header = header_object();
        return MeshHeader::ComputeHeaderSize(header->sub_mesh_count);
    }
    [[nodiscard]]
    MeshHeader * header_object() {
        return reinterpret_cast<MeshHeader *>(asset().ptr);
    }
    [[nodiscard]]
    MeshHeader const * header_object() const {
        return reinterpret_cast<MeshHeader const *>(asset().ptr);
    }
    [[nodiscard]]
    MeshVertex * vertices(MeshHeader::SubMeshIndexType const sub_mesh_index) {
        return vertices_blob(sub_mesh_index).as<MeshVertex>();
    }
    [[nodiscard]]
    MeshVertex const * vertices(MeshHeader::SubMeshIndexType const sub_mesh_index) const {
        return vertices_cblob(sub_mesh_index).as<MeshVertex>();
    }
    [[nodiscard]]
    CBlob vertices_cblob(MeshHeader::SubMeshIndexType const sub_mesh_index) const {
        auto const blob = vertices_blob(sub_mesh_index);
        return CBlob {blob.ptr, blob.len};
    }
    [[nodiscard]]
    CBlob vertices_cblob() const {
        auto const * header = header_object();
        MFA_PTR_ASSERT(header);
        MFA_ASSERT(header->sub_mesh_count > 0);
        MFA_ASSERT(
            header->sub_meshes[0].vertices_offset + 
            header->total_vertex_count * 
            sizeof(Mesh::Data::Vertices::Vertex) == header->sub_meshes[0].indices_offset
        );
        return CBlob {
            asset().ptr + header->sub_meshes[0].vertices_offset,
            header->total_vertex_count * sizeof(Mesh::Data::Vertices::Vertex)
        };
    }
    [[nodiscard]]
    Blob vertices_blob(MeshHeader::SubMeshIndexType const sub_mesh_index) const {
        auto const * header = header_object();
        auto const & sub_mesh = header->sub_meshes[sub_mesh_index];
        return Blob {
            asset().ptr + sub_mesh.vertices_offset,
            sub_mesh.vertex_count * sizeof(Mesh::Data::Vertices::Vertex)
        };
    }
    [[nodiscard]]
    MeshIndex * indices(MeshHeader::SubMeshIndexType const sub_mesh_index) {
        return indices_blob(sub_mesh_index).as<MeshIndex>();
    }
    [[nodiscard]]
    MeshIndex const * indices(MeshHeader::SubMeshIndexType const sub_mesh_index) const {
        return indices_cblob(sub_mesh_index).as<MeshIndex>();
    }
    [[nodiscard]]
    CBlob indices_cblob(MeshHeader::SubMeshIndexType const sub_mesh_index) const {
        auto const blob = indices_blob(sub_mesh_index);
        return CBlob {blob.ptr, blob.len};
    }
    [[nodiscard]]
    CBlob indices_cblob() const {
        auto const * header = header_object();
        MFA_PTR_ASSERT(header);
        MFA_ASSERT(header->sub_mesh_count > 0);
        return CBlob {
            asset().ptr + header->sub_meshes[0].indices_offset,
            header->total_index_count * sizeof(Mesh::Data::Indices::IndexType)
        };
    }
    [[nodiscard]]
    Blob indices_blob(MeshHeader::SubMeshIndexType const sub_mesh_index) const {
        auto const * header = header_object();
        auto const & sub_mesh = header->sub_meshes[sub_mesh_index];
        return Blob {
            asset().ptr + sub_mesh.indices_offset,
            sub_mesh.index_count * sizeof(Mesh::Data::Indices::IndexType)
        };
    }
    [[nodiscard]]
    bool valid() const override {
        return MFA_PTR_VALID(asset().ptr) && header_object()->is_valid();
    }
};*/

//----------------------------------ModelAsset------------------------------------

struct Model {
    std::vector<Texture> textures;
    Mesh mesh;
};

};  // MFA::Asset