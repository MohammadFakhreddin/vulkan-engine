#pragma once

#include "glm/vec3.hpp"

class Ray {
public:

    explicit Ray();

    explicit Ray(glm::vec3 const & origin_, glm::vec3 const & direction_);

    [[nodiscard]]
    glm::vec3 At(float t) const;

    [[nodiscard]]
    glm::vec3 GetOrigin() const;

    [[nodiscard]]
    glm::vec3 GetDirection() const;

private:

    glm::vec3 mOrigin {};
    glm::vec3 mDirection {};

};
