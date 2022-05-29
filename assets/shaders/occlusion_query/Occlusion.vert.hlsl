#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"

struct VSIn {    
    float4 worldPosition;
    float2 baseColorTexCoord : TEXCOORD0;
};

struct VSOut {
    float4 position : SV_POSITION;
    float2 baseColorTexCoord : TEXCOORD0;
};

ConstantBuffer <CameraData> cameraBuffer: register(b0, space0);

VSOut main(VSIn input) {
    VSOut output;
    output.position = mul(cameraBuffer.viewProjection, input.worldPosition);
    output.baseColorTexCoord = input.baseColorTexCoord;
    return output;
}