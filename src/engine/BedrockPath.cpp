#include "BedrockPath.hpp"

#include "BedrockPlatforms.hpp"

namespace MFA::Path {

std::string Asset(char const * address) {
    std::string result;
    Asset(address, result);
    return result;
}

void Asset(char const * address, std::string & outPath) {
#ifdef __DESKTOP__
    char addressBuffer[256] {};
    auto const stringSize = sprintf(addressBuffer, "../assets/%s", address);
    outPath.assign(addressBuffer, stringSize);
#else
    #error "Not implemented"
#endif
}

}