#include "../CameraBuffer.hlsl"

struct VSIn {
    float3 position : SV_POSITION;
    int textureIndex;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

struct VSOut {
    float4 position : SV_POSITION;
    int textureIndex;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

CAMERA_BUFFER_CUSTOM_BINDING(cameraBuffer, b0, space0)

struct PushConsts
{
    float4x4 model;
    uint primitiveIndex;
    float placeholder0;
    float placeholder1;
    float placeholder2;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

VSOut main(VSIn input) {
    VSOut output;

    // Position
    output.position = mul(cameraBuffer.viewProjection, mul(pushConsts.model, float4(input.position, 1.0f)));
    output.textureIndex = input.textureIndex;
    output.uv = input.uv;
    output.color = input.color;

    return output;
}