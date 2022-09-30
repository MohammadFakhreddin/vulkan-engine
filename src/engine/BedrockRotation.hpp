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
        
        bool operator==(glm::vec3 const & eulerAngles) const;

        bool operator==(glm::quat const & quaternion) const;

        bool operator==(float eulerAngles[3]) const;

        bool operator==(Rotation const & rotation) const;

        bool operator!=(glm::vec3 const & eulerAngles) const;

        bool operator!=(glm::quat const & quaternion) const;

        bool operator!=(float eulerAngles[3]) const;

        bool operator!=(Rotation const & rotation) const;

        Rotation & operator=(float eulerAngles[3]);

        Rotation & operator=(glm::vec3 const & eulerAngles);

        Rotation & operator=(glm::quat const & quaternion);

    private:

        glm::quat mQuaternion = glm::identity<glm::quat>();
        glm::vec3 mEulerAngles{0.0f, 0.0f, 0.0f};
        glm::mat4 mMatrix = glm::toMat4(glm::identity<glm::quat>());

    };
}
