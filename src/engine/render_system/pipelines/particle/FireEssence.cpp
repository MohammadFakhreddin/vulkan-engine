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
        std::shared_ptr<RT::GpuTexture> const & fireTexture
        // TODO Smoke texture
    )
        : FireEssence(name, fireTexture, Options {})
    {}
    
    //-------------------------------------------------------------------------------------------------

    FireEssence::FireEssence(
        std::string const & name,
        std::shared_ptr<RT::GpuTexture> const & fireTexture,
        // TODO Smoke texture
        Options options
    )
        : ParticleEssence(createModel(fireTexture), name)
        , mOptions(std::move(options))
    {
        mResizeSignal = RF::AddResizeEventListener([this]()->void {
            computeFirePointSize();
        });
    }

    //-------------------------------------------------------------------------------------------------

    FireEssence::~FireEssence() {
        RF::RemoveResizeEventListener(mResizeSignal);
    }

    //-------------------------------------------------------------------------------------------------

    void FireEssence::update(
        RT::CommandRecordState const & recordState,
        float deltaTimeInSec,
        VariantsList const & variants
    ) {
        
        ParticleEssence::update(recordState, deltaTimeInSec, variants);
        if (mShouldUpdate == false) {
            return;
        } 

        for (int i = 0; i < mOptions.particleCount; ++i)
        {
            auto & vertex = mVertices[i];
            // UpVector is reverse
            glm::vec3 const deltaPosition = -deltaTimeInSec * vertex.speed * Math::UpVector3;
            vertex.localPosition += deltaPosition;
            vertex.localPosition += Math::Random(-mOptions.fireHorizontalMovement[0], mOptions.fireHorizontalMovement[0])
                * deltaTimeInSec * Math::RightVector3;
            vertex.localPosition += Math::Random(-mOptions.fireHorizontalMovement[1], mOptions.fireHorizontalMovement[1])
                * deltaTimeInSec * Math::ForwardVector3;
            
            vertex.remainingLifeInSec -= deltaTimeInSec;
            if (vertex.remainingLifeInSec <= 0)
            {
                vertex.localPosition = vertex.initialLocalPosition;

                vertex.speed = Math::Random(mOptions.particleMinSpeed, mOptions.particleMaxSpeed);
                vertex.remainingLifeInSec = Math::Random(mOptions.particleMinLife, mOptions.particleMaxLife);;
                vertex.totalLifeInSec = vertex.remainingLifeInSec;
            }

            auto const lifePercentage = vertex.remainingLifeInSec / vertex.totalLifeInSec;
            
            vertex.alpha = mOptions.fireAlpha;

            vertex.pointSize = mInitialPointSize * lifePercentage;
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Model> FireEssence::createModel(std::shared_ptr<RT::GpuTexture> const & fireTexture)
    {
        return std::make_shared<AS::Model>(
            createMesh(),
            std::vector<std::shared_ptr<AS::Texture>>{fireTexture}
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Particle::Mesh> FireEssence::createMesh()
    {
        computeFirePointSize();

        auto fireMesh = std::make_shared<AS::Particle::Mesh>(100);
        auto const verticesCount = mOptions.particleCount;
        auto const indicesCount = mOptions.particleCount;
        auto const vertexBuffer = Memory::Alloc(verticesCount * sizeof(AS::Particle::Vertex));
        auto const indexBuffer = Memory::Alloc(indicesCount * sizeof(AS::Index));
        fireMesh->initForWrite(
            verticesCount,
            indicesCount,
            vertexBuffer,
            indexBuffer
        );

        auto * vertexItems = vertexBuffer->memory.as<AS::Particle::Vertex>();
        auto * indexItems = indexBuffer->memory.as<AS::Index>();

        for (int i = 0; i < verticesCount; ++i)
        {
            auto & vertex = vertexItems[i];

            auto const yaw = Math::Random(-Math::PiFloat, Math::PiFloat);
            auto const distanceFromCenter = Math::Random(0.0f, mOptions.fireRadius) * Math::Random(0.5f, 1.0f);
            
            auto transform = glm::identity<glm::mat4>();
            Matrix::RotateWithRadians(transform, glm::vec3{0.0f, yaw, 0.0f});

            glm::vec3 const position = transform * glm::vec4 {distanceFromCenter, 0.0f, 0.0f, 1.0f};

            vertex.localPosition = position;
            vertex.initialLocalPosition = vertex.localPosition;

            vertex.textureIndex = 0;

            vertex.color[0] = 1.0f;
            vertex.color[1] = 0.0f;
            vertex.color[2] = 0.0f;

            vertex.speed = Math::Random(mOptions.particleMinSpeed, mOptions.particleMaxSpeed);
            vertex.remainingLifeInSec = Math::Random(mOptions.particleMinLife, mOptions.particleMaxLife);
            vertex.totalLifeInSec = vertex.remainingLifeInSec;

            auto const lifePercentage = vertex.remainingLifeInSec / vertex.totalLifeInSec;
            
            vertex.alpha = mOptions.fireAlpha;

            vertex.pointSize = mInitialPointSize * lifePercentage;

            indexItems[i] = i;
        }

        mVertices = vertexItems;
        MFA_ASSERT(mVertices != nullptr);

        return fireMesh;
    }

    //-------------------------------------------------------------------------------------------------

    void FireEssence::computeFirePointSize()
    {
        auto const surfaceCapabilities = RF::GetSurfaceCapabilities();
        mInitialPointSize = mOptions.fireInitialPointSize *
            (static_cast<float>(surfaceCapabilities.currentExtent.width) / mOptions.fireTargetExtend[0]);
    }

    //-------------------------------------------------------------------------------------------------

}
