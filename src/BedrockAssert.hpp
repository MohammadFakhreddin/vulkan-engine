#ifndef ASSERT_CLASS
#define ASSERT_CLASS

#if !defined(MFA_OPT_ASSERT_LEVEL)
    #if defined(MFA_DEBUG)
        #define MFA_OPT_ASSERT_LEVEL  2
    #else
        #define MFA_OPT_ASSERT_LEVEL  1
    #endif
#endif

#if defined(_MSC_VER)
    #define MFA_INTERNAL_ASSERT_MACRO(cond_, ...)     ::MFA::Assert::DoAssert((cond_), MFA_STRINGIZE(cond_), __func__, __FILE__, __LINE__, __VA_ARGS__)
#else
    #define MFA_INTERNAL_ASSERT_MACRO(cond_, ...)     ::MFA::Assert::DoAssert((cond_), MFA_STRINGIZE(cond_), __func__, __FILE__, __LINE__, ## __VA_ARGS__)
#endif

#if MFA_OPT_ASSERT_LEVEL == 0
    #define MFA_ASSERT(cond_, ...)        /**/
    #define MFA_ASSERT_STRONG(cond_, ...) /**/
    #define MFA_ASSERT_REPAIR(cond_, ...) (cond_)
#elif MFA_OPT_ASSERT_LEVEL == 1
    #define MFA_ASSERT(cond_, ...)        /**/
    #define MFA_ASSERT_STRONG(cond_, ...) MFA_INTERNAL_ASSERT_MACRO(cond_, __VA_ARGS__)
    #define MFA_ASSERT_REPAIR(cond_, ...) (cond_)
#else   // 2 or higher
    #define MFA_ASSERT(cond_, ...)        MFA_INTERNAL_ASSERT_MACRO(cond_, __VA_ARGS__)
    #define MFA_ASSERT_STRONG(cond_, ...) MFA_INTERNAL_ASSERT_MACRO(cond_, __VA_ARGS__)
    #define MFA_ASSERT_REPAIR(cond_, ...) MFA_INTERNAL_ASSERT_MACRO(cond_, __VA_ARGS__)
#endif

namespace MFA::Assert {

// returns value of "cond"
bool DoAssert (
    bool cond, char const * cond_str,
    char const * func, char const * file, int line,
    char const * msg_fmt = nullptr, ...
);

void DebugBreak ();

} // namespace MFA::Assert

#endif