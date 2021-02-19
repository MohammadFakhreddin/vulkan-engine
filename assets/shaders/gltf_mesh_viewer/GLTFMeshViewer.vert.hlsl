struct VSIn{
    float3 position : POSITION0;
    float2 baseColorTexCood : TEXCOORD0;
    float2 metallicRoughnessTexCood : TEXCOORD1;
    float3 normal: NORMAL0;
};

struct VSOut{
    float4 position : SV_POSITION;
    float2 baseColorTexCood : TEXCOORD0;
    float2 metallicRoughnessTexCood : TEXCOORD1;
    float3 worldPos: POSITION0;
    float3 worldNormal: NORMAL0;
};

struct Transformation {
    float4x4 rotation;
    float4x4 transformation;
    float4x4 projection;
};

ConstantBuffer <Transformation> tBuffer: register(b0, space0);

VSOut main(VSIn input) {
    VSOut output;

    output.worldNormal = mul(tBuffer.rotation, float4(input.normal, 1.0f)).xyz;

    float4 rotationResult = mul(tBuffer.rotation, float4(input.position, 1.0f));
    float4 WorldPos = mul(tBuffer.transformation, rotationResult);

    output.worldPos = WorldPos.xyz;

    output.position = mul(tBuffer.projection, WorldPos);

    return output;
}