#include "BedrockPath.hpp"

#include "BedrockAssert.hpp"

#include <filesystem>
#include <regex>

namespace MFA::Path
{

    struct State
    {
        std::filesystem::path assetsPath {};
    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        state = new State();
        
        char addressBuffer[256]{};
        int stringSize = 0;

#if defined(__PLATFORM_MAC__)
        stringSize = sprintf(addressBuffer, "%s/assets/", GetAssetPath().c_str());
#elif defined(__PLATFORM_WIN__) || defined(__PLATFORM_LINUX__)
        stringSize = sprintf(addressBuffer, "../assets/");
#elif defined(__ANDROID__)
        stringSize = sprintf(addressBuffer, "");
#elif defined(__IOS__)
        stringSize = sprintf(addressBuffer, "%s/assets/", GetAssetPath().c_str());
#else
        #error "Platform not supported"
#endif

        state->assetsPath = std::filesystem::absolute(std::string(addressBuffer, stringSize));
    }

    //-------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    std::string ForReadWrite(std::string const & address)
    {
        std::string result;
        ForReadWrite(address, result);
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    void ForReadWrite(std::string const & address, std::string & outPath)
    {
        outPath = std::filesystem::path(state->assetsPath).append(address).string();
    }

    //-------------------------------------------------------------------------------------------------

    bool RelativeToAssetFolder(std::string const & address, std::string & outRelativePath)
    {
        bool success = false;
        static const std::regex addressRegex(".*assets/.*");
        if (std::regex_match(address.c_str(), addressRegex))
        {
            outRelativePath = std::filesystem::relative(address, state->assetsPath).string();
            success = true;
            if (outRelativePath.empty())
            {
                success = false;
            }
        }
        if (success == false){
            outRelativePath = address;
        }
        return success;
    }

    //-------------------------------------------------------------------------------------------------

}
