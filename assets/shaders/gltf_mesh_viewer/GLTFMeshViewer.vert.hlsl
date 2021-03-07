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
    float3 worldNormal : NORMAL0;
    float3 worldTangent: TEXCOORD3;
    float3 worldBiTangent : TEXCOORD4;
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

	float3 worldTangent = normalize(mul(tBuffer.rotation, input.tangent).xyz);
	float3 worldNormal = normalize(mul(tBuffer.rotation, float4(input.normal, 1.0)).xyz);
	float3 worldBiTangent = normalize(cross(worldNormal.xyz, worldTangent.xyz));
    
    output.worldTangent = worldTangent;
    output.worldNormal = worldNormal;
    output.worldBiTangent = worldBiTangent;

    return output;
}