#include "RayTracingWeekendApplication.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockString.hpp"
#include "engine/BedrockMemory.hpp"
#include "ray/Ray.hpp"
#include "sphere/Sphere.hpp"
#include "engine/BedrockMath.hpp"

#include "glm/glm.hpp"

#include "libs/stb_image/stb_image_write.h"

using namespace MFA;

static float Infinity = std::numeric_limits<float>::infinity();

//-------------------------------------------------------------------------------------------------

glm::vec3 RayTracingWeekendApplication::RayColor(Ray const & ray) {
    glm::vec3 position {};
    glm::vec3 normal {};
    glm::vec3 color {};
    
    bool hit = false;
    
    auto tMax = Infinity;
    
    for (auto & geometry : mGeometries)
    {
        glm::vec3 hitPosition {};
        glm::vec3 hitNormal {};
        glm::vec3 hitColor {};

        if (geometry->HasIntersect(ray, 0.0f, tMax, tMax, hitPosition, hitNormal, hitColor) == true)
        {
            position = hitPosition;
            normal = hitNormal;
            color = hitColor;
            hit = true;
        }
    }

    if (hit) {
        return 0.5f * (normal + 1.0f);  // Moving from -1,1 range to 0 to 1
    }
    
    auto const t = 0.5f * (ray.direction.y + 1.0f);
    return glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.5f, 0.7f, 1.0f), t);
}

//-------------------------------------------------------------------------------------------------

RayTracingWeekendApplication::RayTracingWeekendApplication() = default;

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::run() {
    Init();
    
    MFA_LOG_INFO("Starting to generate image");

    mGeometries.emplace_back(std::make_shared<Sphere>(
        glm::vec3{0.0f, 0.0f, -FocalLength},
        0.5f,
        glm::vec3 {1.0f, 0.0f, 0.0f}
    ));
    
    mGeometries.emplace_back(std::make_shared<Sphere>(
        glm::vec3{0.0f, -100.5f, -FocalLength},
        100.0f,
        glm::vec3 {1.0f, 0.0f, 0.0f}
    ));

    for (int i = 0; i < ImageWidth; ++i) {
        for (int j = 0; j < ImageHeight; ++j) {
            glm::vec3 color {};
            for (int s = 0; s < SampleRate; ++s) {
                auto u = float(i + Math::Random(-1.0f, 1.0f)) / float(ImageWidth - 1.0f);
                auto v = float(j + Math::Random(-1.0f, 1.0f)) / float(ImageHeight - 1.0f);
                auto const ray = mCamera.CreateRay(u, v);
                color += RayColor(ray);
            }
            PutPixel(i, j, color * colorPerSample);
        }
    }

    MFA_LOG_INFO("Image generation is complete");
    
    Shutdown();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::Init() {
    Path::Init();
    
    size_t const memorySize = ImageWidth * ImageHeight * sizeof(uint8_t) * ComponentCount;
    mImageBlob = Memory::Alloc(memorySize);
    mByteArray = mImageBlob->memory.as<uint8_t>();
    memset(mImageBlob->memory.ptr, 0, mImageBlob->memory.len);
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
    auto const pixelIndex = ((ImageHeight - y - 1) * ImageWidth + x) * ComponentCount;

    mByteArray[pixelIndex] += static_cast<uint8_t>(color.r * 255.99f);
    mByteArray[pixelIndex + 1] += static_cast<uint8_t>(color.g * 255.99f);
    mByteArray[pixelIndex + 2] += static_cast<uint8_t>(color.b * 255.99f);
}

//-------------------------------------------------------------------------------------------------
