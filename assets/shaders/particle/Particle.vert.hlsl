#include "../CameraBuffer.hlsl"

struct VSIn {
    // Per vertex data
    float3 localPosition : SV_POSITION;
    int textureIndex;
    float3 color : COLOR0;
    float alpha : COLOR1;
    float pointSize;

    // State variables
    float remainingLifeInSec = 0.0f;
    float totalLifeInSec = 0.0f;
    float placeholder0;
    
    float3 initialLocalPosition{};
    float speed = 0.0f;

    // Per instance data
    float3 instancePosition: SV_POSITION;
};

struct VSOut {
    float4 position : SV_POSITION;
    float2 centerPosition;
    float pointRadius;
    int textureIndex;
    // float2 uv : TEXCOORD0;
    float3 color : COLOR0;
    float alpha : COLOR1;
    [[vk::builtin("PointSize")]] float PSize : PSIZE;
};

ConstantBuffer <CameraData> cameraBuffer: register(b0, space);


VSOut main(VSIn input) {
    VSOut output;

    // Position
    float4 position = mul(cameraBuffer.viewProjection, float4(input.instancePosition + input.localPosition, 1.0f));
    output.position = position;
    // TODO Read more about screen space
    output.centerPosition = ((position.xy / position.w) + 1.0) * 0.5 * cameraBuffer.viewportDimension; // Vertex position in screen space
    output.textureIndex = input.textureIndex;
    // output.uv = input.uv;
    output.color = input.color;
    output.alpha = input.alpha;

    float pointSize = input.pointSize / position.w;
    output.pointRadius = pointSize / 2.0f;
    output.PSize = pointSize;

    return output;
}