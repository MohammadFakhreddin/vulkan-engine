#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"

struct VSIn {    
    float3 localPosition;
};

struct VSOut {
    float4 position : SV_POSITION;
};

ConstantBuffer <CameraData> cameraBuffer: register(b0, space0);

struct PushConsts
{
    float4x4 model;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

VSOut main(VSIn input) {
    VSOut output;
    float4 localPosition = float4(input.localPosition.xyz, 1.0f);
    float4 worldPosition = mul(pushConsts.model, localPosition);
    output.position = mul(cameraBuffer.viewProjection, worldPosition);
    return output;
}