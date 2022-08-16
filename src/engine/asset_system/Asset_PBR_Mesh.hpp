#pragma once

#include "AssetTypes.hpp"
#include "AssetBaseMesh.hpp"

#include <glm/mat4x4.hpp>

#include <string>
#include <vector>
#include <glm/ext/matrix_transform.hpp>

namespace MFA::AssetSystem::PBR
{
    struct Vertex
    {
        Position position{};
        UV baseColorUV{};
        UV normalMapUV{};
        UV metallicUV{};
        UV roughnessUV{};
        UV emissionUV{};
        UV occlusionUV{}; 
        Color color{};
        Normal normalValue{};
        Tangent tangentValue{};
        int hasSkin = 0;       // Duplicate data
        int jointIndices[4]{ 0, 0, 0, 0 };
        float jointWeights[4]{ 0, 0, 0, 0 };
    };

    // TODO Camera

    struct Primitive
    {
        uint32_t uniqueId = 0;                      // Unique id in entire model
        uint32_t vertexCount = 0;
        uint32_t indicesCount = 0;
        uint64_t verticesOffset = 0;                // From start of buffer
        uint64_t indicesOffset = 0;
        uint32_t verticesStartingIndex = 0;
        uint32_t indicesStartingIndex = 0;          // From start of buffer

        // TODO Separate material
        TextureIndex baseColorTextureIndex = 0;
        TextureIndex metallicRoughnessTextureIndex = 0;
        TextureIndex roughnessTextureIndex = 0;
        TextureIndex normalTextureIndex = 0;
        TextureIndex emissiveTextureIndex = 0;
        TextureIndex occlusionTextureIndex = 0;     // Indicates darker area of texture 
        float baseColorFactor[4]{};
        float metallicFactor = 0;                   // Metallic color is stored inside blue
        float roughnessFactor = 0;                  // Roughness color is stored inside green
        float emissiveFactor[3]{};
        //float occlusionStrengthFactor = 0;        // I couldn't find field in tinygltf     
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

        // TODO Use these render variables to render objects in correct order.
        
        AlphaMode alphaMode = AlphaMode::Opaque;
        float alphaCutoff = 0.0f;
        bool doubleSided = false;                   // TODO How are we supposed to render double sided objects ?

        bool hasPositionMinMax = false;

        float positionMin[3]{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        };

        float positionMax[3]{
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min()
        };
    };

    struct SubMesh
    {
        std::vector<Primitive> primitives{};
        std::vector<Primitive *> blendPrimitives{};
        std::vector<Primitive *> maskPrimitives{};
        std::vector<Primitive *> opaquePrimitives{};

        bool hasPositionMinMax = false;

        float positionMin[3]{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        };

        float positionMax[3]{
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min()
        };

        [[nodiscard]]
        std::vector<Primitive *> const & findPrimitives(AlphaMode alphaMode) const;
    };

    struct Node
    {
        friend class Mesh;

        explicit Node();

        explicit Node(Node && other) noexcept;

        explicit Node(Node const & other) noexcept;

        Node & operator=(Node && other) noexcept;

        Node & operator=(Node const & other) noexcept;
        
        int subMeshIndex = -1;
        std::vector<int> children{};
        float transform[16]{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };                       // Preset animation value
        float rotation[4]{ 0.0f, 0.0f, 0.0f, 1.0f };     // x, y, z, w
        float scale[3]{ 1.0f, 1.0f, 1.0f };
        float translate[3]{ 0.0f, 0.0f, 0.0f };
        int parent = -1;
        int skin = -1;

        [[nodiscard]]
        bool hasSubMesh() const noexcept;

        [[nodiscard]]
        bool HasParent() const noexcept;

    private:

        std::atomic<bool> mLock = false;

        bool mIsCachedGlobalTransformValid = false;
        glm::mat4 mCachedGlobalTransform = glm::identity<glm::mat4>();

        bool mIsCachedLocalTransformValid = false;
        glm::mat4 mCachedLocalTransform = glm::identity<glm::mat4>();

    };

    struct Skin
    {
        std::vector<int> joints{};
        std::vector<glm::mat4> inverseBindMatrices{};
        int skeletonRootNode = -1;
    };

    struct Animation
    {

        enum class Path
        {
            Invalid,
            Translation,
            Scale,
            Rotation
        };

        enum class Interpolation
        {
            Invalid,
            Linear,                         // We only support linear for now
            Step,
            CubicSpline
        };

        std::string name{};

        struct Sampler
        {
            struct InputAndOutput
            {
                float input = -1;                           // Input time (Probably in seconds)
                float output[4]{ 0.0f, 0.0f, 0.0f, 0.0f };  // Output can be from 3 to 4 
            };
            Interpolation interpolation{};
            std::vector<InputAndOutput> inputAndOutput{};
        };
        std::vector<Sampler> samplers{};

        struct Channel
        {
            Path path = Path::Invalid;
            uint32_t nodeIndex = 0;         // We might need to store translation, rotation and scale separately
            uint32_t samplerIndex = 0;
        };
        std::vector<Channel> channels{};

        float startTime = -1.0f;
        float endTime = -1.0f;
        float animationDuration{};         // We should track currentTime somewhere else
    };

    struct MeshData
    {
        std::vector<SubMesh> subMeshes{};
        std::vector<Node> nodes{};
        std::vector<Skin> skins{};
        std::vector<Animation> animations{};
        std::vector<uint32_t> rootNodes{};         // Nodes that have no parent

        // We could do this with a T-Pose for more accurate result
        bool hasPositionMinMax = false;

        float positionMin[3]{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        };

        float positionMax[3]{
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min()
        };

        [[nodiscard]]
        bool isValid() const;
    };

    class Mesh final : public MeshBase
    {
    public:

        explicit Mesh();
        ~Mesh() override;

        Mesh(Mesh const &) noexcept = delete;
        Mesh(Mesh &&) noexcept = delete;
        Mesh & operator= (Mesh const & rhs) noexcept = delete;
        Mesh & operator= (Mesh && rhs) noexcept = delete;

        void initForWrite(
            uint32_t vertexCount,
            uint32_t indexCount,
            std::shared_ptr<SmartBlob> const & vertexBuffer,
            std::shared_ptr<SmartBlob> const & indexBuffer
        ) override;

        void finalizeData() override;

        // Returns mesh index
        [[nodiscard]]
        uint32_t insertSubMesh() const;

        void insertPrimitive(
            uint32_t subMeshIndex,
            Primitive & primitive,
            uint32_t vertexCount,
            Vertex * vertices,
            uint32_t indicesCount,
            Index * indices
        );

        [[nodiscard]]
        Node & InsertNode() const;

        [[nodiscard]]
        Skin & InsertSkin() const;

        void insertAnimation(Animation const & animation) const;

        [[nodiscard]]
        bool isValid() const override;

        [[nodiscard]]
        std::shared_ptr<MeshData> const & getMeshData() const;

        void PreparePhysicsPoints(PhysicsPointsCallback const & callback) const override;

    private:

        glm::mat4 ComputeNodeLocalTransform(Node & node) const;

        glm::mat4 ComputeNodeGlobalTransform(Node & node) const;

        void ComputeTriangleMesh(
            Physics::TriangleMeshDesc & triangleMesh,
            glm::mat4 const & matrix,
            Primitive const & primitive
        ) const;

        void ComputeTriangleMeshes(PhysicsPointsCallback const & callback) const;

        std::shared_ptr<MeshData> mData {};

        uint64_t mNextVertexOffset{};
        uint64_t mNextIndexOffset{};
        uint32_t mIndicesStartingIndex{};
        uint32_t mVerticesStartingIndex{};

    };
}

namespace MFA
{
    namespace AS = AssetSystem;
}
