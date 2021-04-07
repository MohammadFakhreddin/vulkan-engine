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
public:
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

    [[nodiscard]]
    Format GetFormat() const noexcept {
        return mFormat;
    }

    [[nodiscard]]
    U16 GetSlices() const noexcept {
        return mSlices;
    }

    [[nodiscard]]
    U8 GetMipCount() const noexcept {
        return mMipCount;
    }

    MipmapInfo const & GetMipmap(U8 const mipLevel) const noexcept {
        return mMipmapInfos[mipLevel];
    }

    [[nodiscard]]
    MipmapInfo const * GetMipmaps() const noexcept {
        return mMipmapInfos.data();
    }

    [[nodiscard]]
    U16 GetDepth() const noexcept {
        return mDepth;
    }

    [[nodiscard]]
    Sampler GetSampler() const noexcept {
        return mSampler;
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
public:
    // TODO Implement serialize and deserialize
    
    using Position = float[3];
    using Normal = float[3];
    using UV = float[2];
    using Color = U8[3];
    using Tangent = float[4]; // XYZW vertex tangents where the w component is a sign value (-1 or +1) indicating handedness of the tangent basis. I need to change XYZ order if handness is different from mine

    using Index = U32;
    struct Vertex {
        Position position;
        UV baseColorUV;
        UV normalMapUV;
        UV metallicUV;
        UV roughnessUV;
        UV emissionUV;
        Color color;
        Normal normalValue;
        Tangent tangentValue;
    };

    // TODO Camera
    
    struct Primitive {
        U32 uniqueId = 0;                      // Unique id in entire model
        U32 vertexCount = 0;
        U32 indicesCount = 0;
        U64 verticesOffset = 0;                // From start of buffer
        U64 indicesOffset = 0;
        U32 indicesStartingIndex = 0;          // From start of buffer
        I16 baseColorTextureIndex = 0;
        I16 mixedMetallicRoughnessOcclusionTextureIndex = 0;
        I16 metallicTextureIndex = 0;
        I16 roughnessTextureIndex = 0;
        I16 normalTextureIndex = 0;
        I16 emissiveTextureIndex = 0;
        float baseColorFactor[4] {};
        float metallicFactor = 0;              // Metallic color is stored inside blue
        float roughnessFactor = 0;             // Roughness color is stored inside green
        float emissiveFactor[3] {};
        bool hasBaseColorTexture = false;
        bool hasMetallicTexture = false;
        bool hasRoughnessTexture = false;
        bool hasEmissiveTexture = false;
        bool hasOcclusionTexture = false;
        bool hasMixedMetallicRoughnessOcclusionTexture = false;
        bool hasNormalBuffer = false;
        bool hasNormalTexture = false;
        bool hasTangentBuffer = false;
    };
    
    struct SubMesh {
        std::vector<Primitive> primitives {};
    };
    
    struct Node {
        int subMeshIndex = -1;
        std::vector<int> children {};
        float transformMatrix[16] {};
        int parent = -1;
        [[nodiscard]]
        bool hasSubMesh() const noexcept {
            return subMeshIndex >= 0;
        }
    };

    void initForWrite(
        U32 vertexCount,
        U32 indexCount,
        const Blob & vertexBuffer,
        const Blob & indexBuffer
    );

    void finalizeData();

    // Returns mesh index
    [[nodiscard]]
    U32 insertSubMesh();

    void insertPrimitive(
        U32 subMeshIndex,
        Primitive && primitive, 
        U32 vertexCount, 
        Vertex * vertices, 
        U32 indicesCount, 
        Index * indices
    );

    void insertNode(Node const & node);

    [[nodiscard]]
    bool isValid() const;

    [[nodiscard]]
    CBlob getVerticesBuffer() const noexcept {
        return mVertexBuffer;
    }

    [[nodiscard]]
    CBlob getIndicesBuffer() const noexcept {
        return mIndexBuffer;
    }

    [[nodiscard]]
    U32 getSubMeshCount() const noexcept {
        return static_cast<U32>(mSubMeshes.size());
    }

    [[nodiscard]]
    SubMesh const & getSubMeshByIndex(U32 const index) const noexcept {
        MFA_ASSERT(index < mSubMeshes.size());
        return mSubMeshes[index];
    }

    [[nodiscard]]
    SubMesh & getSubMeshByIndex(U32 const index) {
        MFA_ASSERT(index < mSubMeshes.size());
        return mSubMeshes[index];
    }

    [[nodiscard]]
    SubMesh const * getSubMeshData() const noexcept {
        return mSubMeshes.data();
    }

    [[nodiscard]]
    Node const & getNodeByIndex(U32 const index) const noexcept {
        MFA_ASSERT(index >= 0);
        MFA_ASSERT(index < mNodes.size());
        return mNodes[index];
    }

    [[nodiscard]]
    U32 getNodesCount() const noexcept {
        return static_cast<U32>(mNodes.size());
    }

    [[nodiscard]]
    Node const * getNodeData() const noexcept {
        return mNodes.data();
    }

    void revokeBuffers(Blob & outVertexBuffer, Blob & outIndexBuffer);

private:
    std::vector<SubMesh> mSubMeshes {};
    std::vector<Node> mNodes {};

    U32 mVertexCount {};
    Blob mVertexBuffer {};

    U32 mIndexCount {};
    Blob mIndexBuffer {};

    U64 mNextVertexOffset {};
    U64 mNextIndexOffset {};
    U32 mNextStartingIndex {};
};

using MeshPrimitive = Mesh::Primitive;
using SubMesh = Mesh::SubMesh;
using MeshNode = Mesh::Node;
using MeshVertex = Mesh::Vertex;
using MeshIndex = Mesh::Index;

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
            mCompiledShaderCode.ptr != nullptr && 
            mCompiledShaderCode.len > 0;
    }

    void init(
        char const * entryPoint,
        Stage const stage,
        Blob const & compiledShaderCode
    ) {
        mEntryPoint = entryPoint;
        mStage = stage;
        mCompiledShaderCode = compiledShaderCode;
    }

    Blob revokeData() {
        auto const buffer = mCompiledShaderCode;
        mCompiledShaderCode = {};
        return buffer;
    }

    [[nodiscard]]
    CBlob getCompiledShaderCode() const noexcept {
        return mCompiledShaderCode;
    }

    [[nodiscard]]
    char const * getEntryPoint() const noexcept {
        return mEntryPoint.c_str();
    }

    [[nodiscard]]
    Stage getStage() const noexcept {
        return mStage;
    }

private:
    std::string mEntryPoint {};     // Ex: main
    Stage mStage = Stage::Invalid;
    Blob mCompiledShaderCode;
};

using ShaderStage = Shader::Stage;

//----------------------------------ModelAsset------------------------------------

struct Model {
    std::vector<Texture> textures;
    Mesh mesh;
};

};  // MFA::Asset