#include "BedrockPath.hpp"

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
//    for (auto const & iterator : std::filesystem::directory_iterator(".")) {
//        MFA_LOG_INFO("%s", iterator.path().c_str());
//    }
#if defined(__PLATFORM_MAC__)
    char addressBuffer[256] {};
    auto const stringSize = sprintf(addressBuffer, "%s/assets/%s", GetAssetPath().c_str(), address);
    outPath.assign(addressBuffer, stringSize);
#elif defined(__PLATFORM_WIN__)
    char addressBuffer[256] {};
    auto const stringSize = sprintf(addressBuffer, "../assets/%s", address);
    outPath.assign(addressBuffer, stringSize);
#elif defined(__ANDROID__)
    char addressBuffer[256] {};
    auto const stringSize = sprintf(addressBuffer, "%s", address);
    outPath.assign(address, stringSize);
#elif defined(__IOS__)
    char addressBuffer[256] {};
    // TODO Check for correctness
    auto const stringSize = sprintf(addressBuffer, "%s/%s", GetAssetPath().c_str(), address);
    outPath.assign(address, stringSize);
#else
#error "Platform not supported"
#endif
}

}
