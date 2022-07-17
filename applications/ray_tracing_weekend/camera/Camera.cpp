#include "Camera.hpp"

#include "glm/glm.hpp"

#include <math.h>

//-------------------------------------------------------------------------------------------------

Camera::Camera(        
    glm::vec3 const & lookFrom,
    glm::vec3 const & lookAt,
    glm::vec3 const & upDirection,
    float vFOV, // In angles
    float aspectRatio,
    float focalLength
) {
    auto viewportHeight = std::tan(glm::radians(vFOV * 0.5f)) * focalLength * 2.0f;
    auto viewportWidth = aspectRatio * viewportHeight;

    auto myForwardDir = glm::normalize(lookAt - lookFrom);
    auto myRightDir = glm::normalize(glm::cross(upDirection, myForwardDir));
    auto myUpDir = glm::normalize(glm::cross(myForwardDir, myRightDir));

    mOrigin = lookFrom;
    mHorizontal = myRightDir * viewportWidth;
    mVertical = myUpDir * viewportHeight;
    mLowerLeftCorner = mOrigin - mHorizontal * 0.5f - mVertical * 0.5f + myForwardDir * focalLength;
}

//-------------------------------------------------------------------------------------------------

Ray Camera::CreateRay(float u, float v) const {
    return Ray(
        mOrigin, 
        mLowerLeftCorner + u * mHorizontal + v * mVertical - mOrigin
    );
}

//-------------------------------------------------------------------------------------------------
