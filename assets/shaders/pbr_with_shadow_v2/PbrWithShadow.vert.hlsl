#include "../DirectionalLightBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"
#include "../CameraBuffer.hlsl"

struct VSIn {
    float4 worldPosition;

    float3 worldNormal;
    float3 worldTangent;
    float3 worldBiTangent;

    float2 baseColorTexCoord : TEXCOORD0;  
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    
    float2 emissiveTexCoord: TEXCOORD4;
    float2 occlusionTexCoord: TEXCOORD5;
};

struct VSOut {
    float4 position : SV_POSITION;
    float3 worldPos: POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    // I think we do not need interpolation on these values
    float3 worldNormal;// : NORMAL0;
    float3 worldTangent;//: TEXCOORD3;
    float3 worldBiTangent;// : TEXCOORD4;

    float2 emissiveTexCoord: TEXCOORD5;
    float2 occlusionTexCoord: TEXCOORD6;

    float4 directionLightPosition[3];
};

CAMERA_BUFFER(cameraBuffer)
DIRECTIONAL_LIGHT(directionalLightBuffer)

VSOut main(VSIn input) {
    VSOut output;

    // Position
    output.position = mul(cameraBuffer.viewProjection, input.worldPosition);
    output.worldPos = input.worldPosition.xyz;

    for (int i = 0; i < directionalLightBuffer.count; ++i)
    {
        output.directionLightPosition[i] = mul(directionalLightBuffer.items[i].viewProjectionMatrix, input.worldPosition);
    }

    // BaseColor
    output.baseColorTexCoord = input.baseColorTexCoord;
    
    // Metallic/Roughness
    output.metallicRoughnessTexCoord = input.metallicRoughnessTexCoord;
    
    // Normals
	output.normalTexCoord = input.normalTexCoord;
    output.worldTangent = input.worldTangent;
    output.worldNormal = input.worldNormal;
    output.worldBiTangent = input.worldBiTangent;

    output.emissiveTexCoord = input.emissiveTexCoord;
    output.occlusionTexCoord = input.occlusionTexCoord;

    return output;
}