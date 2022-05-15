#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"

struct VSIn {    
    float2 baseColorTexCoord : TEXCOORD0;
    float4 worldPosition;
};

struct VSOut {
    float4 position : SV_POSITION;
    float2 baseColorTexCoord : TEXCOORD0;
};

CAMERA_BUFFER(cameraBuffer)

VSOut main(VSIn input) {
    VSOut output;
    output.position = mul(cameraBuffer.viewProjection, input.worldPosition);
    output.baseColorTexCoord = input.baseColorTexCoord;
    return output;
}