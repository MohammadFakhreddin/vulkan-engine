#include "Sphere.hpp"

#include "engine/BedrockAssert.hpp"
#include "geometry/HitRecord.hpp"

#include "glm/glm.hpp"

//-------------------------------------------------------------------------------------------------

Sphere::Sphere(
    glm::vec3 center_, 
    float radius_, 
    std::shared_ptr<Material> material_
)
    : Geometry(std::move(material_))
    , center(std::move(center_))
    , radius(radius_)
    , sqrRadius(radius_ * radius_)
{}

//-------------------------------------------------------------------------------------------------

bool Sphere::HasIntersect(
    Ray const & ray,
    float tMin,
    float tMax,
    HitRecord & hitRecord
)
{
    auto rayOrigin = ray.GetOrigin();
    auto rayDirection = ray.GetDirection();

    auto dir = rayOrigin - center;
    float a = glm::dot(rayDirection, rayDirection);
    float b = glm::dot(rayDirection, dir);
    float c = glm::dot(dir, dir) - sqrRadius;

    auto discriminant = b * b - a * c;
    if (discriminant < 0) {
        return false;
    }

    auto disSqrt = std::sqrt(discriminant);
    
    auto t = (-b - disSqrt) / a;
    
    MFA_ASSERT(tMin <= tMax);
    if (t < tMin || t > tMax) {
        t = (-b + disSqrt) / a;
        if (t < tMin || t > tMax) {
            return false;
        }
    }
    
    hitRecord.t = t;

    hitRecord.position = ray.At(t);
    hitRecord.normal = (hitRecord.position - center) / radius;
    hitRecord.material = material;
    
    if (glm::dot(hitRecord.normal, rayDirection) > 0) {
        // Ray is inside the object
        return false;
    }
    
    return true;
}

//-------------------------------------------------------------------------------------------------
