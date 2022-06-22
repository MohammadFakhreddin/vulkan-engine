#include "Camera.hpp"

Camera::Camera(
    float aspectRatio, 
    float imageWidth, 
    float imageHeight,
    float focalLength
) {
    auto viewportHeight = 2.0;
    auto viewportWidth = aspectRatio * viewportHeight;
    
    mOrigin = glm::vec3(0.0f, 0.0f, 0.0f);
    mHorizontal = glm::vec3(viewportWidth, 0.0f, 0.0f);
    mVertical = glm::vec3(0.0f, viewportHeight, 0.0f);
    mLowerLeftCorner = mOrigin - mHorizontal * 0.5f - mVertical * 0.5f - glm::vec3(0.0f, 0.0f, focalLength);
}

Ray Camera::CreateRay(float u, float v) const {
    return Ray(
        mOrigin, 
        mLowerLeftCorner + u * mHorizontal + v * mVertical - mOrigin
    );
}
