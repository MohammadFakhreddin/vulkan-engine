#pragma once

#include <string>

namespace MFA::Path {

[[nodiscard]]
std::string Asset(char const * address);

void Asset(char const * address, std::string & outPath);

}