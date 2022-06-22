#include "Sphere.hpp"

#include "engine/BedrockAssert.hpp"

#include "glm/glm.hpp"

Sphere::Sphere(glm::vec3 center_, float radius_, glm::vec3 color_)
    : center(std::move(center_))
    , radius(radius_)
    , sqrRadius(radius_ * radius_)
    , color(std::move(color_))
{}

bool Sphere::HasIntersect(
    Ray const & ray,
    float tMin,
    float tMax,
    float & outT,
    glm::vec3 & outPosition,
    glm::vec3 & outNormal,
    glm::vec3 & outColor
)
{
    auto dir = ray.origin - center;
    float a = glm::dot(ray.direction, ray.direction);
    float b = glm::dot(ray.direction, dir);
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
    
    outT = t;

    outPosition = ray.at(t);
    outNormal = (outPosition - center) / radius;
    
    if (glm::dot(outNormal, ray.direction) > 0) {
        // Ray is inside the object
        return false;
    }
    
    outColor = color;
    
    return true;
}
