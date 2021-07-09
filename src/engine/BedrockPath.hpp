#pragma once

#include "BedrockPlatforms.hpp"

#include <string>

namespace MFA::Path {

[[nodiscard]]
std::string Asset(char const * address);

void Asset(char const * address, std::string & outPath);

#if defined(__IOS__) || defined(__PLATFORM_MAC__)
std::string GetAssetPath();
#endif

}
