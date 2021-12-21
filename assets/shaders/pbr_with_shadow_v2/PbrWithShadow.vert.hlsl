#include "../DirectionalLightBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"
#include "../CameraBuffer.hlsl"

struct VSIn {
    float3 position : POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;  
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    float4 tangent: TEXCOORD3;
    float3 normal: NORMAL0;

    float2 emissiveTexCoord: TEXCOORD4;
    float2 occlusionTexCoord: TEXCOORD5;

    int hasSkin;
    int4 jointIndices;
    float4 jointWeights; 
};

struct VSOut {
    float4 position : SV_POSITION;
    float3 worldPos: POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    float3 worldNormal : NORMAL0;
    float3 worldTangent: TEXCOORD3;
    float3 worldBiTangent : TEXCOORD4;

    float2 emissiveTexCoord: TEXCOORD5;
    float2 occlusionTexCoord: TEXCOORD5;

    float4 directionLightPosition[3];
};

CAMERA_BUFFER(cameraBuffer)
DIRECTIONAL_LIGHT(directionalLightBuffer)
SKIN_JOINTS_BUFFER(skinJointsBuffer)

struct PushConsts
{
    float4x4 model;
    float4x4 inverseNodeTransform;
	int skinIndex;
    uint primitiveIndex;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

#define IdentityMat float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

VSOut main(VSIn input) {
    VSOut output;

    float4x4 skinMat;
    if (input.hasSkin == 1) {
        int skinIndex = pushConsts.skinIndex;
        float4x4 inverseNodeTransform = pushConsts.inverseNodeTransform;
        skinMat = 0;
        if (input.jointWeights.x > 0) {
            skinMat += mul(mul(inverseNodeTransform, skinJointsBuffer.joints[skinIndex + input.jointIndices.x]), input.jointWeights.x);
            if (input.jointWeights.y > 0) {
                skinMat += mul(mul(inverseNodeTransform, skinJointsBuffer.joints[skinIndex + input.jointIndices.y]), input.jointWeights.y);
                if (input.jointWeights.z > 0) {
                    skinMat += mul(mul(inverseNodeTransform, skinJointsBuffer.joints[skinIndex + input.jointIndices.z]), input.jointWeights.z);
                    if (input.jointWeights.w > 0) {
                        skinMat += mul(mul(inverseNodeTransform, skinJointsBuffer.joints[skinIndex + input.jointIndices.w]), input.jointWeights.w);
                    }
                }
            }
        }
    } else {
        skinMat = IdentityMat;
    }
    
    float4x4 skinModelMat = mul(pushConsts.model, skinMat);
    
    // Position
    float4 worldPos = mul(skinModelMat, float4(input.position, 1.0f)); // w is 1 because position is a coordinate
    
    output.position = mul(cameraBuffer.viewProjection, worldPos);
    
    output.worldPos = worldPos.xyz;

    for (int i = 0; i < directionalLightBuffer.count; ++i)
    {
        output.directionLightPosition[i] = mul(directionalLightBuffer.items[i].viewProjectionMatrix, float4(worldPos.xyz, 1.0f));
    }

    // BaseColor
    output.baseColorTexCoord = input.baseColorTexCoord;
    
    // Metallic/Roughness
    output.metallicRoughnessTexCoord = input.metallicRoughnessTexCoord;
    
    // Normals
	float4 tempTangent = input.tangent;
    tempTangent = mul(skinModelMat, tempTangent);
    
    float3 worldTangent = normalize(tempTangent.xyz);

	float4 tempNormal = float4(input.normal, 0.0);  // W is zero beacuas normal is a vector
    tempNormal = mul(skinModelMat, tempNormal);
    
    float3 worldNormal = normalize(tempNormal.xyz);
    
	float3 worldBiTangent = normalize(cross(worldNormal.xyz, worldTangent.xyz));
    
    output.normalTexCoord = input.normalTexCoord;
    output.worldTangent = worldTangent;
    output.worldNormal = worldNormal;
    output.worldBiTangent = worldBiTangent;

    output.emissiveTexCoord = input.emissiveTexCoord;
    output.occlusionTexCoord = input.occlusionTexCoord;

    return output;
}