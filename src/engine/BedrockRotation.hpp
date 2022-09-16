#pragma once

#include <glm/gtx/quaternion.hpp>

namespace MFA
{

    class Rotation
    {
    public:

        explicit Rotation();
        explicit Rotation(glm::vec3 const & eulerAngles);
        explicit Rotation(glm::quat const & quaternion);

        [[nodiscard]]
        glm::vec3 const & GetEulerAngles() const;

        [[nodiscard]]
        glm::quat const & GetQuaternion() const;

        [[nodiscard]]
        glm::mat4 const & GetMatrix() const;
        
        void SetEulerAngles(glm::vec3 const & eulerAngles);

        void SetQuaternion(glm::quat const & quaternion);

        bool IsEqual(glm::vec3 const & eulerAngles) const;

        bool IsEqual(glm::quat const & quaternion) const;

        bool IsEqual(float eulerAngles[3]) const;

        bool IsEqual(Rotation const & rotation) const;

        Rotation & Set(float eulerAngles[3]);

        Rotation & Set(glm::vec3 const & eulerAngles);

        Rotation & Set(glm::quat const & quaternion);

    private:

        glm::quat mQuaternion = glm::identity<glm::quat>();
        glm::vec3 mEulerAngles{0.0f, 0.0f, 0.0f};
        glm::mat4 mMatrix = glm::toMat4(glm::identity<glm::quat>());

    };
}
