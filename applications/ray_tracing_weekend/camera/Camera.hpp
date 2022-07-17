#pragma once

#include "ray/Ray.hpp"

#include "glm/vec3.hpp"

class Camera {
public:

    explicit Camera(
        glm::vec3 const & lookFrom,
        glm::vec3 const & lookAt,
        glm::vec3 const & upDirection,
        float vFOV, // In angles
        float aspectRatio,
        float focalLength
    );

    [[nodiscard]]
    Ray CreateRay(float u, float v) const;

private:

    glm::vec3 mOrigin {};
    glm::vec3 mHorizontal {};
    glm::vec3 mVertical {};
    glm::vec3 mLowerLeftCorner {};

};