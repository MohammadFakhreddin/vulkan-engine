#pragma once

#include "BedrockCommon.hpp"

#include <glm/glm.hpp>

#include <vector>

// TODO Separate this file into multiple files

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

    virtual ~Base() = default;

    [[nodiscard]]
    virtual int serialize(CBlob const & writeBuffer);

    // Maybe import BitStream from NSO project
    virtual void deserialize(CBlob const & readBuffer);
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
        uint32_t width = 0;
        uint32_t height = 0;
        uint16_t depth = 0;
    };

    struct MipmapInfo {
        uint64_t offset {};
        uint32_t size {};
        Dimensions dimension {};
    };

    struct SamplerConfig {
        bool isValid = false;
        enum class SampleMode {
            Linear,
            Nearest
        };
        SampleMode sampleMode = SampleMode::Linear;
        uint32_t magFilter = 0;
        uint32_t minFilter = 0;
        uint32_t wrapS = 0;
        uint32_t wrapT = 0;
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

    static uint8_t ComputeMipCount(Dimensions const & dimensions);

    /*
     * This function result is only correct for uncompressed data
     */
    [[nodiscard]]
    static size_t MipSizeBytes (
        Format format,
        uint16_t slices,
        Dimensions const & mipLevelDimension
    );

    // TODO Consider moving this function to util_image
    /*
     * This function result is only correct for uncompressed data
     */
    // NOTE: 0 is the *smallest* mipmap level, and "mip_count - 1" is the *largest*.
    [[nodiscard]]
    static Dimensions MipDimensions (
        uint8_t mipLevel,
        uint8_t mipCount,
        Dimensions originalImageDims
    );

    /*
    * This function result is only correct for uncompressed data
    * Returns space required for both mipmaps and TextureHeader
    */
    [[nodiscard]]
    static size_t CalculateUncompressedTextureRequiredDataSize(
        Format format,
        uint16_t slices,
        Dimensions const & dims,
        uint8_t mipCount
    );

    void initForWrite(
        Format format,
        uint16_t slices,
        uint16_t depth,
        SamplerConfig const * sampler,
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
    Blob revokeBuffer();

    [[nodiscard]]
    CBlob GetBuffer() const noexcept;

    [[nodiscard]]
    Format GetFormat() const noexcept;

    [[nodiscard]]
    uint16_t GetSlices() const noexcept;

    [[nodiscard]]
    uint8_t GetMipCount() const noexcept;

    [[nodiscard]]
    MipmapInfo const & GetMipmap(uint8_t const mipLevel) const noexcept;

    [[nodiscard]]
    MipmapInfo const * GetMipmaps() const noexcept;

    [[nodiscard]]
    uint16_t GetDepth() const noexcept {
        return mDepth;
    }

    void SetSampler(SamplerConfig * sampler);

    [[nodiscard]]
    SamplerConfig const & GetSampler() const noexcept;

private:
    Format mFormat = Format::INVALID;
    uint16_t mSlices = 0;
    uint8_t mMipCount = 0;
    uint16_t mDepth = 0;
    SamplerConfig mSampler {};
    std::vector<MipmapInfo> mMipmapInfos {};
    Blob mBuffer {};
    uint64_t mCurrentOffset = 0;
    int mPreviousMipWidth = -1;
    int mPreviousMipHeight = -1;
};

using TextureFormat = Texture::Format;
using SamplerConfig = Texture::SamplerConfig;

//---------------------------------MeshAsset-------------------------------------
class Mesh final : public Base {
public:
    // TODO Implement serialize and deserialize
    
    using Position = float[3];
    using Normal = float[3];
    using UV = float[2];
    using Color = uint8_t[3];
    using Tangent = float[4]; // XYZW vertex tangents where the w component is a sign value (-1 or +1) indicating handedness of the tangent basis. I need to change XYZ order if handness is different from mine

    using Index = uint32_t;
    struct Vertex {
        Position position {};
        UV baseColorUV {};
        UV normalMapUV {};
        UV metallicUV {};
        UV roughnessUV {};
        UV emissionUV {};
        Color color {};
        Normal normalValue {};
        Tangent tangentValue {};
        int hasSkin = 0;       // Duplicate data
        int jointIndices[4] {0, 0, 0, 0};
        float jointWeights[4] {0, 0, 0, 0};
    };

    // TODO Camera
    
    struct Primitive {
        uint32_t uniqueId = 0;                      // Unique id in entire model
        uint32_t vertexCount = 0;
        uint32_t indicesCount = 0;
        uint64_t verticesOffset = 0;                // From start of buffer
        uint64_t indicesOffset = 0;
        uint32_t indicesStartingIndex = 0;          // From start of buffer
        int16_t baseColorTextureIndex = 0;
        int16_t metallicRoughnessTextureIndex = 0;
        int16_t roughnessTextureIndex = 0;
        int16_t normalTextureIndex = 0;
        int16_t emissiveTextureIndex = 0;
        float baseColorFactor[4] {};
        float metallicFactor = 0;              // Metallic color is stored inside blue
        float roughnessFactor = 0;             // Roughness color is stored inside green
        float emissiveFactor[3] {};
        bool hasBaseColorTexture = false;
        bool hasMetallicTexture = false;
        bool hasRoughnessTexture = false;
        bool hasEmissiveTexture = false;
        bool hasOcclusionTexture = false;
        bool hasMetallicRoughnessTexture = false;
        bool hasNormalBuffer = false;
        bool hasNormalTexture = false;
        bool hasTangentBuffer = false;
        bool hasSkin = false;
    };
    
    struct SubMesh {
        std::vector<Primitive> primitives {};
    };
    
    struct Node {
        int subMeshIndex = -1;
        std::vector<int> children {};
        float transform[16]{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };                       // Preset animation value
        float rotation[4] {0.0f, 0.0f, 0.0f, 1.0f};     // x, y, z, w
        float scale[3] {1.0f, 1.0f, 1.0f};                  
        float translate[3] {0.0f, 0.0f, 0.0f};
        int parent = -1;
        int skin = -1;
        
        [[nodiscard]]
        bool hasSubMesh() const noexcept {
            return subMeshIndex >= 0;
        }
    };

    struct Skin {
        std::vector<int> joints {};
        std::vector<glm::mat4> inverseBindMatrices {};
        int skeletonRootNode = -1;
    };

    struct Animation {

        enum class Path {
            Invalid,
            Translation,
            Scale,
            Rotation
        };

        enum class Interpolation {
            Invalid,
            Linear,                         // We only support linear for now
            Step,
            CubicSpline
        };

        std::string name {};

        struct Sampler {
            struct InputAndOutput {
                float input = -1;                           // Input time (Probably in seconds)
                float output [4] {0.0f, 0.0f, 0.0f, 0.0f};  // Output can be from 3 to 4 
            };
            Interpolation interpolation {};
            std::vector<InputAndOutput> inputAndOutput {};
        };
        std::vector<Sampler> samplers {};

        struct Channel {
            Path path = Path::Invalid;      
            uint32_t nodeIndex = 0;         // We might need to store translation, rotation and scale separately
            uint32_t samplerIndex = 0;
        };
        std::vector<Channel> channels {};

        float startTime = -1.0f;
        float endTime = -1.0f;
        float animationDuration {};         // We should track currentTime somewhere else
    };

    void InitForWrite(
        uint32_t vertexCount,
        uint32_t indexCount,
        const Blob & vertexBuffer,
        const Blob & indexBuffer
    );

    void FinalizeData();

    // Returns mesh index
    [[nodiscard]]
    uint32_t InsertSubMesh();

    void InsertPrimitive(
        uint32_t subMeshIndex,
        Primitive & primitive, 
        uint32_t vertexCount, 
        Vertex * vertices, 
        uint32_t indicesCount, 
        Index * indices
    );

    void InsertNode(Node const & node);

    void InsertSkin(Skin const & skin);

    void InsertAnimation(Animation const & animation);

    [[nodiscard]]
    bool IsValid() const;

    [[nodiscard]]
    CBlob GetVerticesBuffer() const noexcept;

    [[nodiscard]]
    CBlob GetIndicesBuffer() const noexcept;

    [[nodiscard]]
    uint32_t GetSubMeshCount() const noexcept;

    [[nodiscard]]
    SubMesh const & GetSubMeshByIndex(uint32_t const index) const noexcept;

    [[nodiscard]]
    SubMesh & GetSubMeshByIndex(uint32_t const index);

    [[nodiscard]]
    SubMesh const * GetSubMeshData() const noexcept {
        return mSubMeshes.data();
    }

    [[nodiscard]]
    Node const & GetNodeByIndex(uint32_t const index) const noexcept;

    [[nodiscard]]
    Node & GetNodeByIndex(uint32_t const index);

    [[nodiscard]]
    uint32_t GetNodesCount() const noexcept {
        return static_cast<uint32_t>(mNodes.size());
    }

    [[nodiscard]]
    Node const * GetNodeData() const noexcept {
        return mNodes.data();
    }

    [[nodiscard]]
    Skin const & GetSkinByIndex(uint32_t const index) const noexcept;

    [[nodiscard]]
    uint32_t GetSkinsCount() const noexcept {
        return static_cast<uint32_t>(mSkins.size());
    }

    [[nodiscard]]
    Skin const * GetSkinData() const noexcept {
        return mSkins.data();
    }

    [[nodiscard]]
    Animation const & GetAnimationByIndex(uint32_t const index) const noexcept;

    [[nodiscard]]
    uint32_t GetAnimationsCount() const noexcept {
        return static_cast<uint32_t>(mAnimations.size()); 
    }

    [[nodiscard]]
    Animation const * GetAnimationData() const noexcept {
        return mAnimations.data();
    }

    [[nodiscard]]
    uint32_t GetRootNodesCount() const {
        return static_cast<uint32_t>(mRootNodes.size());
    }

    [[nodiscard]]
    uint32_t const * GetRootNodesData() {
        return mRootNodes.data();
    }

    [[nodiscard]]
    uint32_t GetIndexOfRootNode(uint32_t const index) const;

    [[nodiscard]]
    Node & GetRootNodeByIndex(uint32_t const index);

    [[nodiscard]]
    Node const & GetRootNodeByIndex(uint32_t const index) const;

    void RevokeBuffers(Blob & outVertexBuffer, Blob & outIndexBuffer);

private:

    std::vector<SubMesh> mSubMeshes {};
    std::vector<Node> mNodes {};
    std::vector<Skin> mSkins {};
    std::vector<Animation> mAnimations {};
    std::vector<uint32_t> mRootNodes {};         // Nodes that have no parent

    uint32_t mVertexCount {};
    Blob mVertexBuffer {};

    uint32_t mIndexCount {};
    Blob mIndexBuffer {};

    uint64_t mNextVertexOffset {};
    uint64_t mNextIndexOffset {};
    uint32_t mNextStartingIndex {};

};

using MeshPrimitive = Mesh::Primitive;
using SubMesh = Mesh::SubMesh;
using MeshNode = Mesh::Node;
using MeshVertex = Mesh::Vertex;
using MeshIndex = Mesh::Index;
using MeshSkin = Mesh::Skin;
using MeshAnimation = Mesh::Animation;

//--------------------------------ShaderAsset--------------------------------------
class Shader final : public Base {
public:
    enum class Stage {
        Invalid,
        Vertex,
        Geometry,
        Fragment,
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
    std::vector<Texture> textures {};
    Mesh mesh {};
};

};  // MFA::Asset

namespace MFA {
    namespace AS = AssetSystem;
}
