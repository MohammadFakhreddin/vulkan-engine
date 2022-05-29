#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"
#include "../DirectionalLightBuffer.hlsl"

struct VSIn {
    float4 worldPosition;
};

struct VSOut {
    float4 position : SV_POSITION;
    int layer : SV_RenderTargetArrayIndex;
};

// DIRECTIONAL_LIGHT(directionalLightBuffer)
ConstantBuffer <DirectionalLightBufferData> directionalLightBuffer: register(b1, space0);

struct PushConsts
{   
    int lightIndex;
    int placeholder0;
    int placeholder1;
    int placeholder2;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

VSOut main(VSIn input) {
    VSOut output;

    float4x4 directionalLightMat = directionalLightBuffer.items[pushConsts.lightIndex].viewProjectionMatrix;

    // Position
    output.position = mul(directionalLightMat, input.worldPosition);
    output.layer = pushConsts.lightIndex;
    return output;
}