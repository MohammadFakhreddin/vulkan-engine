#pragma once

#include <string>

#define MFA_STRING(variable, format, ...)           \
{                                                   \
    char charBufer[200] {};                         \
    auto const strLength = sprintf(                 \
        charBufer,                                  \
        format,                                     \
        __VA_ARGS__                                 \
    );                                              \
    variable.assign(charBufer, strLength);          \
}                                                   \
