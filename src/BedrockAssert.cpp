#include "BedrockAssert.hpp"

#include <cstdarg>
#include <cstdio>
#include <exception>        // std::terminate()

#define MFA_OPT_ASSERT_TO_STDERR      1
#define MFA_OPT_ASSERT_DEBUGBREAK     1
#define MFA_OPT_ASSERT_TERMINATE      1

namespace MFA::Assert {

static char user_str [1024 + 1];
static char msgbox_str [2048 + 1];

bool DoAssert (
    bool cond, char const * cond_str,
    char const * func, char const * file, int line,
    char const * msg_fmt /*= nullptr*/, ...
) {
    if (!cond) {
    #if defined(MFA_OPT_ASSERT_TERMINATE)
        bool should_terminate = true;
    #else
        bool should_terminate = false;
    #endif

    #if defined(MFA_OPT_ASSERT_TO_STDERR) || defined(MFA_OPT_ASSERT_TO_MSGBOX)
        user_str[0] = '\0';
        if (msg_fmt) {
            va_list args;
            va_start(args, msg_fmt);
            ::vsnprintf(user_str, sizeof(user_str) - 1, msg_fmt, args);
            va_end(args);
            user_str[sizeof(user_str) - 1] = '\0';
        }
    #endif

    #if defined(MFA_OPT_ASSERT_TO_STDERR)
        ::fprintf(stderr, 
            "\n"
            "+--------------------------------------------------------------------------------\n"
            "| [ASSERTION FAILURE] In \"%s\" (line #%d of %s)\n"
            "|     Condition:  %s\n"
            "%s%s%s"
            "+--------------------------------------------------------------------------------\n"
            , func, line, file, cond_str
            , msg_fmt ? "|   Description:  " : ""
            , msg_fmt ? user_str : ""
            , msg_fmt ? "\n|\n" : ""
        );
        ::fflush(stderr);
    #endif
    
    #if defined(MFA_OPT_ASSERT_DEBUGBREAK)
        __debugbreak();
    #endif

        if (should_terminate)
            std::terminate();
    }
    return cond;
}

void DebugBreak () {
    __debugbreak();
}

}