#pragma once

#include "ray/Ray.hpp"
#include "material/Material.hpp"

#include "glm/vec3.hpp"

#include <memory>

class Material;

struct HitRecord;

class Geometry {
public:

    explicit Geometry(std::shared_ptr<Material> material_);

    [[nodiscard]]
    virtual bool HasIntersect(
        Ray const & ray,
        float tMin,
        float tMax,
        HitRecord & outHitRecord
    ) = 0;

    std::shared_ptr<Material> const material = nullptr;
};
