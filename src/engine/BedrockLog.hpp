#pragma once

#define MFA_RAW_LOG(level_, channel_, fmt_, ...)  ::MFA::Log::PrintFmt(__FILE__, __LINE__, __FUNCTION__, level_, channel_, fmt_ __VA_OPT__(,) __VA_ARGS__)
#define MFA_LOG_INFO()
#define MFA_LOG_WARN()
#define MFA_LOG_ERROR()
#define MFA_LOG_FATAL()

// TODO Actual PrintFmt also remove fucking VA_OPT