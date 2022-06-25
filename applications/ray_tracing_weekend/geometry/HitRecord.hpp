#pragma once

#include "glm/vec3.hpp"

#include <memory>

class Material;

struct HitRecord {
    float t;
    glm::vec3 position;
    glm::vec3 normal;
    std::shared_ptr<Material> material;
};
