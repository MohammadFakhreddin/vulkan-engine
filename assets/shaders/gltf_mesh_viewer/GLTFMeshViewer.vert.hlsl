struct VSIn{
    float3 position : POSITION0;
    float2 baseColorTexCoord : TEXCOORD0;
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    float2 normalTexCoord: TEXCOORD2;
    float4 tangent: TEXCOORD3;
    float3 normal: NORMAL0;
};

struct VSOut{
    float4 position : SV_POSITION;
    float2 baseColorTexCoord : TEXCOORD0;
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    float2 normalTexCoord: TEXCOORD2;
    float3 worldPos: POSITION0;
    float3 normal : NORMAL0;
    float4 tangent: TEXCOORD3;
};

struct Transformation {
    float4x4 rotation;
    float4x4 transformation;
    float4x4 projection;
};

ConstantBuffer <Transformation> tBuffer: register(b0, space0);

VSOut main(VSIn input) {
    VSOut output;

    float4 rotationResult = mul(tBuffer.rotation, float4(input.position, 1.0f));
    float4 worldPos = mul(tBuffer.transformation, rotationResult);

    output.position = mul(tBuffer.projection, worldPos);
    output.baseColorTexCoord = input.baseColorTexCoord;
    output.metallicRoughnessTexCoord = input.metallicRoughnessTexCoord;
    output.normalTexCoord = input.normalTexCoord;
    output.worldPos = worldPos.xyz;

    output.normal = input.normal;
    output.tangent = input.tangent;

    return output;
}