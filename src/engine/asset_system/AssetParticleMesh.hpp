#pragma once

#include "AssetBaseMesh.hpp"

#include <glm/vec3.hpp>

namespace MFA::AssetSystem::Particle
{
    // TODO Add rotation
    struct Vertex
    {
        glm::vec3 localPosition{};
        int textureIndex = -1;
        //float uv [2]{};
        float color [3]{};
        float alpha = 1.0f;
        float pointSize = 10.0f;
        // These variable currently used only on cpu and not on gpu
        float remainingLifeInSec = 0.0f;
        float totalLifeInSec = 0.0f;
        glm::vec3 initialLocalPosition{};
        float speed = 0.0f;
    };

    struct PerInstanceData
    {
        float instancePosition[3] {};
    };

    class Mesh final : public MeshBase
    {
    public:
        /*
         *  @maxInstanceCount_ Maximum instance we are going to create from this model (Important for essence)
         */
        explicit Mesh(uint32_t maxInstanceCount_);
        ~Mesh() override;

        Mesh(Mesh const &) noexcept = delete;
        Mesh(Mesh &&) noexcept = delete;
        Mesh & operator= (Mesh const & rhs) noexcept = delete;
        Mesh & operator= (Mesh && rhs) noexcept = delete;

        uint32_t const maxInstanceCount;

    };

}

namespace MFA
{
    namespace AS = AssetSystem;
}
