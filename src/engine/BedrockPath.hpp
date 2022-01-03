#pragma once

#include "BedrockPlatforms.hpp"

#include <string>

namespace MFA::Path
{
    void Init();

    void Shutdown();

    [[nodiscard]]
    std::string ForReadWrite(char const * address);

    void ForReadWrite(char const * address, std::string & outPath);

    bool RelativeToAssetFolder(char const * address, std::string & outRelativePath);

#if defined(__IOS__) || defined(__PLATFORM_MAC__)
    std::string GetAssetPath();
#endif

}
