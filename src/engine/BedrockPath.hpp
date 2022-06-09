#pragma once

#include "BedrockPlatforms.hpp"

#include <string>

namespace MFA::Path
{
    void Init();

    void Shutdown();

    [[nodiscard]]
    std::string ForReadWrite(std::string const & address);

    void ForReadWrite(std::string const & address, std::string & outPath);

    // Returns true if relative path was extracted
    bool RelativeToAssetFolder(std::string const & address, std::string & outAddress);

    [[nodiscard]]
    std::string RelativeToAssetFolder(std::string const & address);

    [[nodiscard]]
    std::string ExtractDirectoryFromPath(std::string const & path);

    [[nodiscard]]
    std::string ExtractExtensionFromPath(std::string const & path);


#if defined(__IOS__) || defined(__PLATFORM_MAC__)
    std::string GetAssetPath();
#endif

}
