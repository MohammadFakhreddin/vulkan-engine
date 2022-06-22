#pragma once

#include "engine/BedrockMemory.hpp"

#include <string>

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

    static constexpr char const * OutputFile = "output.jpeg";
    static constexpr float AspectRatio = 16.0f / 9.0f;
    static constexpr int ImageWidth = 400.0f;
    static constexpr int ImageHeight = static_cast<int>(ImageWidth / AspectRatio);
    static constexpr int ComponentCount = 3;
    static constexpr int Quality = 100;

    std::string mFilePath {};
    std::shared_ptr<MFA::SmartBlob> mImageBlob = nullptr;
    uint8_t * mByteArray = nullptr;

};
