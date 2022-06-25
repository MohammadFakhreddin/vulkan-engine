#include "Diffuse.hpp"

#include "geometry/HitRecord.hpp"
#include "engine/bedrockMatrix.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

bool Diffuse::Scatter(
    Ray const & ray,
    HitRecord const & hitRecord,
    glm::vec3 & outAttenuation,
    Ray & outScatteredRay
) const {
    outAttenuation = color;
    auto direction = hitRecord.normal + RandomUnitVector();
    if (Matrix::IsNearZero(direction)) {
        direction = hitRecord.normal;
    }
    outScatteredRay = Ray {hitRecord.position, direction};
    return true;   
}

//-------------------------------------------------------------------------------------------------
