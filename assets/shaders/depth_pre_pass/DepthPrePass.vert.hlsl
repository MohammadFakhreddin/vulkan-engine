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

CAMERA_BUFFER(cameraBuffer)

VSOut main(VSIn input) {
    VSOut output;

    // Position
    output.position = mul(cameraBuffer.viewProjection, input.worldPosition);

    output.baseColorTexCoord = input.baseColorTexCoord;

    return output;
}