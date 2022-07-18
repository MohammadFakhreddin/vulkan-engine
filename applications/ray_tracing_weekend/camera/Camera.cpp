#include "Camera.hpp"

#include "engine/BedrockMath.hpp"

#include "glm/gtx/norm.hpp"

#include <math.h>

using namespace MFA;

//-------------------------------------------------------------------------------------------------

static glm::vec3 RandomUnitDisk(){
    while(true)
    {
        glm::vec3 const randomDisk {
            Math::Random(-1.0f, 1.0f), 
            Math::Random(-1.0f, 1.0f), 
            0.0f
        };
        if (glm::length2(randomDisk) > 1.0f) {
            continue;
        }
        return randomDisk;
    }
    return glm::vec3 {};
 }

//-------------------------------------------------------------------------------------------------

Camera::Camera(        
    glm::vec3 const & lookFrom,
    glm::vec3 const & lookAt,
    glm::vec3 const & upDirection,
    float vFOV, // In angles
    float aspectRatio,
    float aperture,
    float focusDist
) {
    auto viewportHeight = std::tan(glm::radians(vFOV * 0.5f)) * focusDist * 2.0f;
    auto viewportWidth = aspectRatio * viewportHeight;

    auto myForwardDir = glm::normalize(lookAt - lookFrom);
    auto myRightDir = glm::normalize(glm::cross(upDirection, myForwardDir));
    auto myUpDir = glm::normalize(glm::cross(myForwardDir, myRightDir));

    mOrigin = lookFrom;
    mHorizontal = myRightDir * viewportWidth;
    mVertical = myUpDir * viewportHeight;
    mLowerLeftCorner = mOrigin - mHorizontal * 0.5f - mVertical * 0.5f + myForwardDir * focusDist;

    mLensRadius = aperture * 0.5f;
}

//-------------------------------------------------------------------------------------------------

Ray Camera::CreateRay(float u, float v) const {
    auto const offset = RandomUnitDisk() * mLensRadius;
    return Ray(
        mOrigin + offset, 
        mLowerLeftCorner + u * mHorizontal + v * mVertical - mOrigin - offset
    );
}

//-------------------------------------------------------------------------------------------------
