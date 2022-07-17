#include "RayTracingWeekendApplication.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockString.hpp"
#include "engine/BedrockMemory.hpp"
#include "ray/Ray.hpp"
#include "geometry/sphere/Sphere.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "geometry/HitRecord.hpp"
#include "material/diffuse/Diffuse.hpp"
#include "material/metal/Metal.hpp"
#include "material/dielectric/Dielectric.hpp"

#include "glm/glm.hpp"

#include "libs/stb_image/stb_image_write.h"

#include <memory.h>

using namespace MFA;

static float Infinity = std::numeric_limits<float>::infinity();

//-------------------------------------------------------------------------------------------------

glm::vec3 RayTracingWeekendApplication::RayColor(Ray const & ray, int maxDepth) {
    // glm::vec3 position {};
    // glm::vec3 normal {};
    // glm::vec3 color {};
    
    // bool hit = false;

    if (maxDepth <= 0.0f) {
        return glm::vec3 {};
    }
    
    auto tMax = Infinity;

    HitRecord hitRecord {};
    hitRecord.t = tMax;

    bool hasHit = false;
    
    for (auto & geometry : mGeometries)
    {
        if (geometry->HasIntersect(
            ray, 
            0.001f,             // T min
            hitRecord.t,        // T max
            hitRecord) == true)
        {
            hasHit = true;
        }
    }

    if (hasHit) {
        glm::vec3 attenuation {};
        Ray scatteredRay;
        bool hasScatteredRay = hitRecord.material->Scatter(ray, hitRecord, attenuation, scatteredRay);
        if (hasScatteredRay)
        {
            return attenuation * RayColor(scatteredRay, maxDepth - 1);
        }
        return glm::vec3 {0.0f, 0.0f, 0.0f};
    }
    
    auto const t = 0.5f * (ray.GetDirection().y + 1.0f);
    return glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.5f, 0.7f, 1.0f), t);
}

//-------------------------------------------------------------------------------------------------

RayTracingWeekendApplication::RayTracingWeekendApplication() = default;

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::run() {
    Init();
    
    MFA_LOG_INFO("Starting to generate image");
    
    Camera camera {
        glm::vec3{ 2, 2, 1 }, 
        glm::vec3{ 0, 0, -1 }, 
        MFA::Math::UpVector3, 
        20,
        AspectRatio, 
        FocalLength
    };
    
    auto redDiffMat = std::make_shared<Diffuse>(glm::vec3 {0.7f, 0.0f, 0.0f});
    auto blueDiffMat = std::make_shared<Diffuse>(glm::vec3 {0.0f, 0.0f, 0.7f});
    auto metal1 = std::make_shared<Metal>(glm::vec3 {0.8f, 0.8f, 0.8f}, 0.1f);
    auto metal2 = std::make_shared<Metal>(glm::vec3 {0.8f, 0.6f, 0.2f}, 0.5f);
    auto dielectric1 = std::make_shared<Dielectric>(glm::vec3 {1.0f, 1.0f, 1.0f}, 1.5f);

    mGeometries.emplace_back(std::make_shared<Sphere>(
        glm::vec3{0.0f, 0.0f, -FocalLength},
        0.5f,
        redDiffMat
    ));
    
    mGeometries.emplace_back(std::make_shared<Sphere>(
        glm::vec3{0.0f, -100.5f, -FocalLength},
        100.0f,
        blueDiffMat
    ));

    mGeometries.emplace_back(std::make_shared<Sphere>(
        glm::vec3{-1.0f, 0.0f, -FocalLength},
        0.5f,
        metal1
    ));

    mGeometries.emplace_back(std::make_shared<Sphere>(
        glm::vec3{1.0f, 0.0f, -FocalLength},
        0.5f,
        dielectric1
    ));

    for (int i = 0; i < ImageWidth; ++i) {
        for (int j = 0; j < ImageHeight; ++j) {
            JS::AssignTask([this, i, j, &camera](int num, int count)->void {
                glm::vec3 color {};
                for (int s = 0; s < SampleRate; ++s) {
                    auto u = (static_cast<float>(i) + Math::Random(-1.0f, 1.0f)) / float(ImageWidth - 1.0f);
                    auto v = (static_cast<float>(j) + Math::Random(-1.0f, 1.0f)) / float(ImageHeight - 1.0f);
                    auto const ray = camera.CreateRay(u, v);
                    color += RayColor(ray, MaxDepth);
                }
                color *= ColorPerSample;
                // reinhard tone mapping
                color = color / (color + glm::vec3(1.0f));
                // Gamma correct
                color = pow(color, glm::vec3(GammaCorrection)); 
                PutPixel(i, j, color);
            });
        }
    }

    JS::WaitForThreadsToFinish();

    MFA_LOG_INFO("Image generation is complete");
    
    Shutdown();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::Init() {
    Path::Init();
    JS::Init();
    
    size_t const memorySize = static_cast<size_t>(ImageWidth * ImageHeight) * sizeof(uint8_t) * ComponentCount;
    mImageBlob = Memory::Alloc(memorySize);
    mByteArray = mImageBlob->memory.as<uint8_t>();
    memset(mImageBlob->memory.ptr, 0, mImageBlob->memory.len);
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::Shutdown()
{
    WriteToFile();

    JS::Shutdown();
    Path::Shutdown();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::WriteToFile() const
{
    auto const filePath = Path::ForReadWrite("output.jpeg");
    auto const result = stbi_write_jpg(
        filePath.c_str(),
        static_cast<int>(ImageWidth),
        static_cast<int>(ImageHeight),
        ComponentCount,
        mImageBlob->memory.ptr,
        Quality
    );
    MFA_ASSERT(result == 1);
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::PutPixel(int x, int y, glm::vec3 const & color)
{
    auto const pixelIndex = ((static_cast<int>(ImageHeight) - y - 1) * static_cast<int>(ImageWidth) + x) * ComponentCount;

    mByteArray[pixelIndex] = static_cast<uint8_t>(color.r * 255.99f);
    mByteArray[pixelIndex + 1] = static_cast<uint8_t>(color.g * 255.99f);
    mByteArray[pixelIndex + 2] = static_cast<uint8_t>(color.b * 255.99f);
}

//-------------------------------------------------------------------------------------------------
