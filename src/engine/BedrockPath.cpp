#include "BedrockPath.hpp"

#include "BedrockPlatforms.hpp"

namespace MFA::Path {

std::string Asset(char const * address) {
    #ifdef __DESKTOP__
        char addressBuffer[256] {};
        sprintf(addressBuffer, "../assets/%s", address);
        return std::string(addressBuffer);
    #else
        #error "Not implemented"
    #endif
}

}