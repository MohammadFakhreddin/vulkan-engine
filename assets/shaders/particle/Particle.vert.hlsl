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

const float PointSize = 10.0f;

CAMERA_BUFFER_CUSTOM_BINDING(cameraBuffer, b0, space0)

VSOut main(VSIn input) {
    VSOut output;

    // Position
    float4 position = mul(cameraBuffer.viewProjection, float4(input.instancePosition + input.localPosition, 1.0f));
    output.position = position;
    output.textureIndex = input.textureIndex;
    output.uv = input.uv;
    output.color = input.color;
    output.alpha = input.alpha;

    output.PSize = PointSize / position.w;

    return output;
}