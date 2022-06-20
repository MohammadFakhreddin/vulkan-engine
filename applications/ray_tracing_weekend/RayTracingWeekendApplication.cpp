#include "RayTracingWeekendApplication.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockString.hpp"
#include "engine/BedrockMemory.hpp"
#include "ray/Ray.hpp"
#include "sphere/Sphere.hpp"

#include "glm/glm.hpp"

#include "libs/stb_image/stb_image_write.h"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

static glm::vec3 RayColor(Ray const & ray) {
    auto const t = 0.5f * (ray.direction.y + 1.0f);
    return glm::mix(glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), t);
}

//-------------------------------------------------------------------------------------------------

RayTracingWeekendApplication::RayTracingWeekendApplication() = default;

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::run() {
    Init();
    
    // TODO: Why!
    auto viewportHeight = 2.0;
    auto viewportWidth = AspectRatio * viewportHeight;
    auto focalLength = 1.0f;

    auto origin = glm::vec3(0.0f, 0.0f, 0.0f);
    auto horizontal = glm::vec3(viewportWidth, 0.0f, 0.0f);
    auto vertical = glm::vec3(0.0f, viewportHeight, 0.0f);
    auto lowerLeftCorner = origin - horizontal * 0.5f - vertical * 0.5f - glm::vec3(0.0f, 0.0f, focalLength);

    Sphere sphere {glm::vec3{0.0f, 0.0f, -focalLength}, 0.5f, glm::vec3 {1.0f, 0.0f, 0.0f}};

    for (int i = 0; i < ImageWidth; ++i) {
        for (int j = 0; j < ImageHeight; ++j) {

            auto u = float(i) / float(ImageWidth - 1.0f);
            auto v = float(j) / float(ImageHeight - 1.0f);

            Ray const ray (origin, lowerLeftCorner + u * horizontal + v * vertical - origin);

            glm::vec3 position {};
            glm::vec3 normal {};
            glm::vec3 color {};

            if (sphere.HasIntersect(ray, position, normal, color) == false)
            {
                color = RayColor(ray);
            } else {
                color = 0.5f * (normal + 1.0f); // Moving from -1,1 range to 0 to 1
            }

            PutPixel(i, j, color);

        }
    }
    
    Shutdown();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::Init() {
    Path::Init();
    size_t const memorySize = ImageWidth * ImageHeight * sizeof(uint8_t) * ComponentCount;
    mImageBlob = Memory::Alloc(memorySize);
    mByteArray = mImageBlob->memory.as<uint8_t>();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::Shutdown()
{
    WriteToFile();
    Path::Shutdown();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::WriteToFile() const
{
    auto const filePath = Path::ForReadWrite("output.jpeg");
    auto const result = stbi_write_jpg(
        filePath.c_str(),
        ImageWidth,
        ImageHeight,
        ComponentCount,
        mImageBlob->memory.ptr,
        Quality
    );
    MFA_ASSERT(result == 1);
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::PutPixel(int x, int y, glm::vec3 const & color)
{
    auto const pixelIndex = (y * ImageWidth + x) * ComponentCount;

    mByteArray[pixelIndex] = static_cast<uint8_t>(color.r * 255.99f);
    mByteArray[pixelIndex + 1] = static_cast<uint8_t>(color.g * 255.99f);
    mByteArray[pixelIndex + 2] = static_cast<uint8_t>(color.b * 255.99f);
}

//-------------------------------------------------------------------------------------------------
