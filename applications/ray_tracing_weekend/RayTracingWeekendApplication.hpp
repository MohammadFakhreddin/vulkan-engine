#pragma once
#include "engine/BedrockMemory.hpp"

class RayTracingWeekendApplication 
{
public:

    explicit RayTracingWeekendApplication();

    void run();

private:

    void Init();

    void Shutdown();

    void WriteToFile() const;

    static constexpr char const * OutputFile = "output.jpeg";
    static constexpr int ImageWidth = 256;
    static constexpr int ImageHeight = 256;
    static constexpr int ComponentCount = 3;
    static constexpr int Quality = 10;

    std::string mFilePath {};
    std::shared_ptr<MFA::SmartBlob> mImageBlob = nullptr;
    uint8_t * mByteArray = nullptr;

};
