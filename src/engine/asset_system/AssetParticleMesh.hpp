#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "engine/BedrockMath.hpp"

namespace MFA::AssetSystem::Particle
{
    // TODO Add rotation
    struct Vertex
    {
        // Per vertex data
        float localPosition[3] {};
        int textureIndex = -1;
        float color[3] {};
        float alpha  = 1.0f;
        float pointSize = 1.0f;

        // State variables
        float remainingLifeInSec = 0.0f;
        float totalLifeInSec = 0.0f;
        float placeholder0 = 0.0f;
        
        float initialLocalPosition[3] {};
        float speed = 0.0f;

        // Per instance data
        float instancePosition[3] {};
    };

    struct PerInstanceData
    {
        float instancePosition[3] {};
    };

    struct Params {
        int count = 512;
        glm::vec3 moveDirection = Math::UpVector3;
        
        float minLife = 1.0f;
        float maxLife = 1.5f;
        float minSpeed = 1.0f;
        float maxSpeed = 2.0f;

        float radius = 0.7f;
        glm::vec3 noiseMin {-1.0f, -1.0f, -1.0f};

        float alpha = 0.0001f;    
        glm::vec3 noiseMax {1.0f, 1.0f, 1.0f};

        float pointSize = 0.0f;
        glm::vec3 placeholder {};
    };

}

namespace MFA
{
    namespace AS = AssetSystem;
}
