#include "BedrockPath.hpp"

#include "BedrockPlatforms.hpp"
#include "BedrockAssert.hpp"

#include <filesystem>

namespace MFA::Path {

std::string Asset(char const * address) {
    std::string result;
    Asset(address, result);
    MFA_ASSERT(std::filesystem::exists(result));
    return result;
}

void Asset(char const * address, std::string & outPath) {
#if defined(__PLATFORM_MAC__)
////    auto iterator = std::filesystem::directory_iterator(".");
////    std::filesystem::directory_iterator endIterator;
////    while (iterator != endIterator) {
////        printf("%s\n", iterator->path().c_str());
////        iterator++;
////    }
    char addressBuffer[256] {};
    auto const stringSize = sprintf(addressBuffer, "../../assets/%s", address);
    outPath.assign(addressBuffer, stringSize);
#elif defined(__PLATFORM_WIN__)
    char addressBuffer[256] {};
    auto const stringSize = sprintf(addressBuffer, "../assets/%s", address);
    outPath.assign(addressBuffer, stringSize);
#elif defined(__ANDROID__)
    char addressBuffer[256] {};
    auto const stringSize = sprintf(addressBuffer, "%s", address);
    outPath.assign(address, stringSize);
#else
#error "Platform not supported"
#endif
}

}
