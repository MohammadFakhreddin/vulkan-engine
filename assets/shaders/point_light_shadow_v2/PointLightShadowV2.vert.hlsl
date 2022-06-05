#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"
#include "../PointLightBuffer.hlsl"

struct VSIn {
    float4 worldPosition;
};

struct VSOut {
    float4 position: SV_POSITION;
    float4 worldPosition;
    int layer : SV_RenderTargetArrayIndex;
};

ConstantBuffer <CameraData> cameraBuffer: register(b0, space0);

ConstantBuffer <PointLightsBufferData> pointLightsBuffer: register(b2, space0);

struct PushConsts
{   
    int lightIndex;
    int faceIndex;
    int placeholder0;
    int placeholder1;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

VSOut main(VSIn input) {
    VSOut output;
    
    float4x4 viewProjectionMat = pointLightsBuffer
        .items[pushConsts.lightIndex]
        .viewProjectionMatrices[pushConsts.faceIndex];

    output.position = mul(viewProjectionMat, input.worldPosition);
    output.worldPosition = input.worldPosition;
    output.layer = pushConsts.lightIndex * 6 + pushConsts.faceIndex;
    
    return output;
}