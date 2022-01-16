#include "../CameraBuffer.hlsl"

struct VSIn {
    // Per vertex data
    float3 localPosition : SV_POSITION;
    int textureIndex;
    float2 uv : TEXCOORD0;
    float3 color : COLOR0;
    float alpha : COLOR1;
    // Per instance data
    float3 instancePosition: SV_POSITION;
};

struct VSOut {
    float4 position : SV_POSITION;
    int textureIndex;
    float2 uv : TEXCOORD0;
    float3 color : COLOR0;
    float alpha : COLOR1;
    [[vk::builtin("PointSize")]] float PSize : PSIZE;
};

#define POINT_SIZE 10;

CAMERA_BUFFER_CUSTOM_BINDING(cameraBuffer, b0, space0)

VSOut main(VSIn input) {
    VSOut output;

    // Position
    output.position = mul(cameraBuffer.viewProjection, float4(input.instancePosition + input.localPosition, 1.0f));
    output.textureIndex = input.textureIndex;
    output.uv = input.uv;
    output.color = input.color;

    // output.PSize = mul(cameraBuffer.viewProjection, POINT_SIZE);
    output.PSize = POINT_SIZE;  // TODO We should apply view projection to point size

    return output;
}