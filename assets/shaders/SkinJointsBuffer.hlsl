#ifndef SKIN_JOINTS_BUFFER_HLSL
#define SKIN_JOINTS_BUFFER_HLSL

struct SkinJoints {
    float4x4 joints[];
};

#define SKIN_JOINTS_BUFFER(bufferName)                              \
ConstantBuffer <SkinJoints> bufferName: register(b0, space2);       \

#endif