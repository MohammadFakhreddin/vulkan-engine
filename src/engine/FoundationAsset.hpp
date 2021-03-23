#pragma once

#include "BedrockMatrix.hpp"
#include "BedrockCommon.hpp"

#include <vector>

namespace MFA::AssetSystem {

enum class AssetType {
    INVALID     = 0,
    Texture     = 1,
    Mesh        = 2,
    Shader      = 3,
    Material    = 4
};

//-----------------------------------Asset-------------------------------
class Base {
public:
    [[nodiscard]]
    virtual int serialize(CBlob const & writeBuffer) {
        MFA_CRASH("Not implemented");
    }   // Maybe import BitStream from NSO project
    virtual void deserialize(CBlob const & readBuffer) {
        MFA_CRASH("Not implemented");
    }
};

//----------------------------------Texture------------------------------
class Texture final : public Base {
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
        Dimensions dimension {};
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

    static U8 ComputeMipCount(Dimensions const & dimensions);

    /*
     * This function result is only correct for uncompressed data
     */
    [[nodiscard]]
    static size_t MipSizeBytes (
        Format format,
        U16 slices,
        Dimensions const & mipLevelDimension
    );

    // TODO Consider moving this function to util_image
    /*
     * This function result is only correct for uncompressed data
     */
    // NOTE: 0 is the *smallest* mipmap level, and "mip_count - 1" is the *largest*.
    [[nodiscard]]
    static Dimensions MipDimensions (
        U8 mip_level,
        U8 mip_count,
        Dimensions original_image_dims
    );

    /*
    * This function result is only correct for uncompressed data
    * Returns space required for both mipmaps and TextureHeader
    */
    [[nodiscard]]
    static size_t CalculateUncompressedTextureRequiredDataSize(
        Format format,
        U16 slices,
        Dimensions const & dims,
        U8 mipCount
    );

    void initForWrite(
        Format format,
        U16 slices,
        U8 mipCount,
        U16 depth,
        Sampler const * sampler,
        Blob const & buffer
    );

    void addMipmap(
        Dimensions const & dimension,
        CBlob const & data
    );

    [[nodiscard]]
    size_t mipOffsetInBytes(uint8_t mip_level, uint8_t slice_index = 0) const;

    [[nodiscard]]
    bool isValid() const;

    [[nodiscard]]
    Blob revokeBuffer() {
        auto const buffer = mBuffer;
        mBuffer = {};
        return buffer;
    }

    [[nodiscard]]
    CBlob GetBuffer() const noexcept {
        return mBuffer;
    }

private:
    Format mFormat = Format::INVALID;
    U16 mSlices = 0;
    U8 mMipCount = 0;
    U16 mDepth = 0;
    Sampler mSampler {};
    std::vector<MipmapInfo> mMipmapInfos {};
    Blob mBuffer {};
    U32 mCurrentOffset = 0;
};

using TextureFormat = Texture::Format;
using TextureSampler = Texture::Sampler;

//---------------------------------MeshAsset-------------------------------------
class Mesh final : public Base {
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

    void insertVertex(Vertex const & vertex);

    void insertIndex(IndexType const & index);

    void insertMesh(SubMesh const & subMesh);

    void insertNode(Node const & node);

    [[nodiscard]]
    bool isValid() const;

    [[nodiscard]]
    Vertex const * getVerticesData() const noexcept {
        return mVertices.data();
    }

    [[nodiscard]]
    U32 getVerticesCount() const noexcept {
        return static_cast<U32>(mVertices.size());
    }

    [[nodiscard]]
    IndexType const * getIndicesData() const noexcept {
        return mIndices.data();
    }

    [[nodiscard]]
    U32 getIndicesCount() const noexcept {
        return static_cast<U32>(mIndices.size());
    }

    [[nodiscard]]
    SubMesh const * getSubMeshData() const noexcept {
        return mSubMeshes.data();
    }

    [[nodiscard]]
    U32 getSubMeshSize() const noexcept {
        return static_cast<U32>(mSubMeshes.size());
    }

    [[nodiscard]]
    Node const * getNodeData() const noexcept {
        return mNodes.data();
    }

    [[nodiscard]]
    U32 getNodeSize() const noexcept {
        return static_cast<U32>(mNodes.size());
    }

    void revokeData();

private:
    std::vector<Vertex> mVertices {};
    std::vector<IndexType> mIndices {};
    std::vector<SubMesh> mSubMeshes {};
    std::vector<Node> mNodes {};

};

//--------------------------------ShaderAsset--------------------------------------
class Shader final : public Base {
public:
    enum class Stage {
        Invalid,
        Vertex,
        Fragment
    };

    [[nodiscard]]
    bool isValid() const {
        return mEntryPoint.empty() == false && 
            Stage::Invalid != mStage && 
            mData.ptr != nullptr && 
            mData.len > 0;
    }

    void init(
        char const * entryPoint,
        Stage const stage,
        Blob const & data
    ) {
        mEntryPoint = entryPoint;
        mStage = stage;
        mData = data;
    }

    Blob revokeData() {
        auto const buffer = mData;
        mData = {};
        return buffer;
    }

    [[nodiscard]]
    CBlob GetData() const noexcept {
        return mData;
    }


private:
    std::string mEntryPoint {};     // Ex: main
    Stage mStage = Stage::Invalid;
    Blob mData;
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