#include "FireEssence.hpp"

#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
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
        : ParticleEssence(name, params.count, maxInstanceCount, std::move(fireTextures))
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

    FireEssence::~FireEssence()
    {
        RF::RemoveResizeEventListener(mResizeSignal);
    }

    //-------------------------------------------------------------------------------------------------

    void FireEssence::init()
    {
        computePointSize();

        auto const verticesCount = mParams.count;
        auto const indicesCount = mParams.count;
        auto const vertexBuffer = Memory::Alloc(verticesCount * sizeof(AS::Particle::Vertex));
        auto const indexBuffer = Memory::Alloc(indicesCount * sizeof(AS::Index));

        auto * vertices = vertexBuffer->memory.as<AS::Particle::Vertex>();
        auto * indices = indexBuffer->memory.as<AS::Index>();

        // Creating initial data

        for (int i = 0; i < verticesCount; ++i)
        {
            auto & vertex = vertices[i];

            auto const yaw = Math::Random(-Math::PiFloat, Math::PiFloat);
            auto const distanceFromCenter = Math::Random(0.0f, mParams.radius) * Math::Random(0.5f, 1.0f);

            auto transform = glm::identity<glm::mat4>();
            Matrix::RotateWithRadians(transform, glm::vec3{ 0.0f, yaw, 0.0f });

            glm::vec3 const position = transform * glm::vec4{ distanceFromCenter, 0.0f, 0.0f, 1.0f };

            Matrix::CopyGlmToCells(position, vertex.localPosition);
            Matrix::CopyGlmToCells(position, vertex.initialLocalPosition);
            
            vertex.textureIndex = 0;

            vertex.color[0] = 1.0f;
            vertex.color[1] = 0.0f;
            vertex.color[2] = 0.0f;

            vertex.speed = Math::Random(mParams.minSpeed, mParams.maxSpeed);
            vertex.remainingLifeInSec = Math::Random(mParams.minLife, mParams.maxLife);
            vertex.totalLifeInSec = vertex.remainingLifeInSec;

            auto const lifePercentage = vertex.remainingLifeInSec / vertex.totalLifeInSec;

            vertex.alpha = mParams.alpha;

            vertex.pointSize = mInitialPointSize * lifePercentage;

            indices[i] = i;
        }
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
