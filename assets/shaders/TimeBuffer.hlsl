#ifndef TIME_BUFFER_HLSL
#define TIME_BUFFER_HLSL

struct TimeGroup {
    int timeInMs;
    float timeInSec;
    // Maybe delta time
};

#define TIME_BUFFER(bufferName)                                     \
ConstantBuffer<TimeGroup> bufferName : register(b3, space0);        \

#endif