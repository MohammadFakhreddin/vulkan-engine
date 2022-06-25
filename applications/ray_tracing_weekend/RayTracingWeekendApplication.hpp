#pragma once

#include "engine/BedrockMemory.hpp"
#include "geometry/Geometry.hpp"
#include "camera/Camera.hpp"

#include <string>
#include <vector>

#include <glm/vec3.hpp>

class RayTracingWeekendApplication 
{
public:

    explicit RayTracingWeekendApplication();

    void run();

private:

    void Init();

    void Shutdown();

    void WriteToFile() const;
    
    void PutPixel(int x, int y, glm::vec3 const & color);

    [[nodiscard]]
    glm::vec3 RayColor(Ray const & ray, int maxDepth);

    static constexpr char const * OutputFile = "output.jpeg";
    static constexpr float AspectRatio = 16.0f / 9.0f;
    static constexpr float ImageWidth = 400.0f;
    static constexpr float ImageHeight = static_cast<int>(ImageWidth / AspectRatio);
    static constexpr float FocalLength = 1.0f;
    static constexpr int ComponentCount = 3;
    static constexpr int Quality = 100;
    static constexpr int SampleRate = 100.0f;
    static constexpr float ColorPerSample = 1.0f / static_cast<float>(SampleRate);
    static constexpr int MaxDepth = 50;
    static constexpr float GammaCorrection = 1.0f / 2.2f;

    std::string mFilePath {};
    std::shared_ptr<MFA::SmartBlob> mImageBlob = nullptr;
    uint8_t * mByteArray = nullptr;

    std::vector<std::shared_ptr<Geometry>> mGeometries {};

    Camera mCamera {AspectRatio, ImageWidth, ImageHeight, FocalLength};

};
