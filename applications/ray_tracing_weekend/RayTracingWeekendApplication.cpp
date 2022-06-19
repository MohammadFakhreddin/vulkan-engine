#include "RayTracingWeekendApplication.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockString.hpp"
#include "engine/BedrockMemory.hpp"

#include "libs/stb_image/stb_image_write.h"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

RayTracingWeekendApplication::RayTracingWeekendApplication() = default;

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::run() {
    Init();
    
    {
        for (int j = ImageHeight - 1; j >= 0; --j) {
            for (int i = 0; i < ImageWidth; ++i) {
                /*auto r = double(i) / (ImageWidth - 1);
                auto g = double(j) / (ImageHeight - 1);
                auto b = 0.25;

                int ir = static_cast<int>(255.999 * r);
                int ig = static_cast<int>(255.999 * g);
                int ib = static_cast<int>(255.999 * b);*/

                auto const pixelIndex = (j * ImageWidth + i) * ComponentCount;

                mByteArray[pixelIndex] = static_cast<uint8_t>(ImageWidth);
                mByteArray[pixelIndex + 1] = static_cast<uint8_t>(ImageHeight);
                mByteArray[pixelIndex + 2] = static_cast<uint8_t>(pixelIndex);
            }
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
