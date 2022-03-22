#include "FireEssence.hpp"

#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/RenderTypes.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

        
    FireEssence::FireEssence(
        std::string const & name,
        uint32_t const maxInstanceCount,
        std::vector<std::shared_ptr<RT::GpuTexture>> fireTextures,
        FireParams fireParams,
        AS::Particle::Params params
    )
        : ParticleEssence(name, maxInstanceCount, std::move(fireTextures))
        , mFireParams(std::move(fireParams))
        , mParams(std::move(params))
    {
        computePointSize();
        mResizeSignal = RF::AddResizeEventListener([this]()->void
        {
            computePointSize();
        });
    }

    //-------------------------------------------------------------------------------------------------

    //FireEssence::FireEssence(
    //    std::string const & name,
    //    std::shared_ptr<RT::GpuTexture> const & fireTexture,
    //    // TODO Smoke texture
    //    Options const & options
    //)
    //    : ParticleEssence(prepareConstructorParams(fireTexture, name, options))
    //    , mOptions(options)
    //{

    //    computePointSize();
    //    mResizeSignal = RF::AddResizeEventListener([this]()->void
    //    {
    //        computePointSize();
    //    });

    //    mVertices = mMesh->getVertexData()->memory.as<AS::Particle::Vertex>();
    //}

    //-------------------------------------------------------------------------------------------------

    FireEssence::~FireEssence()
    {
        RF::RemoveResizeEventListener(mResizeSignal);
    }

    //-------------------------------------------------------------------------------------------------

    //void FireEssence::update(
    //    float deltaTimeInSec,
    //    VariantsList const & variants
    //)
    //{

    //    ParticleEssence::update(deltaTimeInSec, variants);
    //    if (mShouldUpdate == false)
    //    {
    //        return;
    //    }

    //    for (int i = 0; i < mOptions.particleCount; ++i)
    //    {
    //        auto & vertex = mVertices[i];
    //        // UpVector is reverse
    //        glm::vec3 const deltaPosition = -deltaTimeInSec * vertex.speed * Math::UpVector3;
    //        vertex.localPosition += deltaPosition;
    //        vertex.localPosition += Math::Random(-mOptions.fireHorizontalMovement[0], mOptions.fireHorizontalMovement[0])
    //            * deltaTimeInSec * Math::RightVector3;
    //        vertex.localPosition += Math::Random(-mOptions.fireHorizontalMovement[1], mOptions.fireHorizontalMovement[1])
    //            * deltaTimeInSec * Math::ForwardVector3;

    //        vertex.remainingLifeInSec -= deltaTimeInSec;
    //        if (vertex.remainingLifeInSec <= 0)
    //        {
    //            vertex.localPosition = vertex.initialLocalPosition;

    //            vertex.speed = Math::Random(mOptions.particleMinSpeed, mOptions.particleMaxSpeed);
    //            vertex.remainingLifeInSec = Math::Random(mOptions.particleMinLife, mOptions.particleMaxLife);;
    //            vertex.totalLifeInSec = vertex.remainingLifeInSec;
    //        }

    //        auto const lifePercentage = vertex.remainingLifeInSec / vertex.totalLifeInSec;

    //        vertex.alpha = mOptions.fireAlpha;

    //        vertex.pointSize = mInitialPointSize * lifePercentage;
    //    }
    //}

    //-------------------------------------------------------------------------------------------------

    //FireEssence::Params FireEssence::prepareConstructorParams(
    //    std::shared_ptr<RT::GpuTexture> const & fireTexture,
    //    std::string const & name,
    //    Options const & options
    //)
    //{
    //    auto const mesh = createMesh(options);

    //    auto meshBuffers = RF::CreateMeshBuffers(*mesh);
    //    std::vector textures{ fireTexture };

    //    Params params{
    //        .gpuModel = std::make_shared<RT::GpuModel>(
    //            name,
    //            std::move(meshBuffers),
    //            std::move(textures)
    //        ),
    //        .mesh = mesh
    //    };
    //    return params;
    //}

    //-------------------------------------------------------------------------------------------------

    void FireEssence::init()
    {
        computePointSize();

        auto const verticesCount = mParams.count;
        auto const indicesCount = mParams.count;
        auto const vertexBuffer = Memory::Alloc(verticesCount * sizeof(AS::Particle::Vertex));
        auto const indexBuffer = Memory::Alloc(indicesCount * sizeof(AS::Index));
        //fireMesh->initForWrite(
        //    verticesCount,
        //    indicesCount,
        //    vertexBuffer,
        //    indexBuffer
        //);

        auto * vertexItems = vertexBuffer->memory.as<AS::Particle::Vertex>();
        auto * indexItems = indexBuffer->memory.as<AS::Index>();

        for (int i = 0; i < verticesCount; ++i)
        {
            auto & vertex = vertexItems[i];

            auto const yaw = Math::Random(-Math::PiFloat, Math::PiFloat);
            auto const distanceFromCenter = Math::Random(0.0f, options.fireRadius) * Math::Random(0.5f, 1.0f);

            auto transform = glm::identity<glm::mat4>();
            Matrix::RotateWithRadians(transform, glm::vec3{ 0.0f, yaw, 0.0f });

            glm::vec3 const position = transform * glm::vec4{ distanceFromCenter, 0.0f, 0.0f, 1.0f };

            vertex.localPosition = position;
            vertex.initialLocalPosition = position;

            vertex.textureIndex = 0;

            vertex.color[0] = 1.0f;
            vertex.color[1] = 0.0f;
            vertex.color[2] = 0.0f;

            vertex.speed = Math::Random(options.particleMinSpeed, options.particleMaxSpeed);
            vertex.remainingLifeInSec = Math::Random(options.particleMinLife, options.particleMaxLife);
            vertex.totalLifeInSec = vertex.remainingLifeInSec;

            auto const lifePercentage = vertex.remainingLifeInSec / vertex.totalLifeInSec;

            vertex.alpha = options.fireAlpha;

            vertex.pointSize = mInitialPointSize * lifePercentage;

            indexItems[i] = i;
        }

        mVertices = vertexItems;
        MFA_ASSERT(mVertices != nullptr);

        return fireMesh;
    }

    //-------------------------------------------------------------------------------------------------

    void FireEssence::computePointSize()
    {
        auto const surfaceCapabilities = RF::GetSurfaceCapabilities();

        AS::Particle::Params newParams {};
        memcpy(&newParams, &mParams, sizeof(mParams));
        
        newParams.pointSize = mFireParams.initialPointSize * (static_cast<float>(surfaceCapabilities.currentExtent.width) / mFireParams.targetExtend[0]);
        
        updateParamsIfChanged(mParams);
    }

    //-------------------------------------------------------------------------------------------------

    void FireEssence::updateParamsIfChanged(AS::Particle::Params const & newParams)
    {
        if (memcmp(&newParams, &mParams, sizeof(mParams)) == 0)
        {
            return;
        }
        memcpy(&mParams, &newParams, sizeof(mParams));
        updateParamsBuffer(mParams);
    }

    //-------------------------------------------------------------------------------------------------

}
