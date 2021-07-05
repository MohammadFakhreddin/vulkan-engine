#pragma once

#include "BedrockPlatforms.hpp"

#include <string>

namespace MFA::Path {

[[nodiscard]]
std::string Asset(char const * address);

void Asset(char const * address, std::string & outPath);

#ifdef __IOS__
std::string GetAssetPath();
#endif

}
