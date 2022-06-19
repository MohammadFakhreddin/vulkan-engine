#include "RayTracingWeekendApplication.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockString.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

RayTracingWeekendApplication::RayTracingWeekendApplication() {

}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::run() {
    Init();
    
    {
        auto fileHandle = FS::OpenFile(Path::GetAssetPath() + "output.ppm", FS::Usage::Write);
        MFA_ASSERT(fileHandle != nullptr);
            
        std::string data = "";
        
        // Image

        const int imageWidth = 256;
        const int imageHeight = 256;
        
        MFA_APPEND(data, "P3\n %d %d\n255\n", imageWidth, imageHeight)

        for (int j = imageHeight - 1; j >= 0; --j) {
            for (int i = 0; i < imageWidth; ++i) {
                auto r = double(i) / (imageWidth - 1);
                auto g = double(j) / (imageHeight - 1);
                auto b = 0.25;

                int ir = static_cast<int>(255.999 * r);
                int ig = static_cast<int>(255.999 * g);
                int ib = static_cast<int>(255.999 * b);
                
                MFA_APPEND(data, "%d %d %d\n", ir, ig, ib);
            }
        }
        
        auto size = data.length() * sizeof(char);

        auto writeCount = fileHandle->write(CBlob {data.c_str(), size});
        MFA_ASSERT(writeCount == size);
    }
    
    Shutdown();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::Init() {
    Path::Init();
}

//-------------------------------------------------------------------------------------------------

void RayTracingWeekendApplication::Shutdown()
{
    Path::Shutdown();
}

//-------------------------------------------------------------------------------------------------
