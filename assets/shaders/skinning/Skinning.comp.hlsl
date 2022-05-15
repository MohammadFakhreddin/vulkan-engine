#include "../TimeBuffer.hlsl"

struct RawVertex {
    float4 localPosition;

    float4 tangent;
    
    float3 normal;
    int hasSkin;

    int4 jointIndices;

    float4 jointWeights; 
};

struct SkinnedVertex // Per variant
{
    float4 worldPosition;
    
    float3 worldNormal;
    float placeholder1;
    
    float3 worldTangent;
    float placeholder2;

    float3 worldBiTangent;
    float placeholder3;
};

struct SkinJoints {
    float4x4 joints[];
};

struct PushConsts
{   
    float4x4 model;
    float4x4 inverseNodeTransform;
    int skinIndex;
    int vertexCount;
    int placeholder0;
    int placeholder1;
};

StructuredBuffer <RawVertex> rawVertices : register(b0, space1);
ConstantBuffer <SkinJoints> skinJoints: register(b0, space2);
RWStructuredBuffer<SkinnedVertex> skinnedVertices : register(u1, space2);

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

#define IdentityMat float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

#define rawVertex rawVertices[index]
#define skinnedVertex skinnedVertices[index]

[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint index = GlobalInvocationID.x;
    if (index >= pushConsts.vertexCount)
    {
        return;
    }

    float4x4 skinMat;
    if (rawVertex.hasSkin == 1) {
        int skinIndex = pushConsts.skinIndex;
        float4x4 inverseNodeTransform = pushConsts.inverseNodeTransform;
        skinMat = 0;
        if (rawVertex.jointWeights.x > 0) {
            
            skinMat += mul(
                mul(
                    inverseNodeTransform, 
                    skinJoints.joints[skinIndex + rawVertex.jointIndices.x]
                ), 
                rawVertex.jointWeights.x
            );

            if (rawVertex.jointWeights.y > 0) {
            
                skinMat += mul(
                    mul(
                        inverseNodeTransform, 
                        skinJoints.joints[skinIndex + rawVertex.jointIndices.y]
                    ), 
                    rawVertex.jointWeights.y
                );
            
                if (rawVertex.jointWeights.z > 0) {
            
                    skinMat += mul(
                        mul(
                            inverseNodeTransform, 
                            skinJoints.joints[skinIndex + rawVertex.jointIndices.z]
                        ), 
                        rawVertex.jointWeights.z
                    );
            
                    if (rawVertex.jointWeights.w > 0) {
                        skinMat += mul(
                            mul(
                                inverseNodeTransform, 
                                skinJoints.joints[skinIndex + rawVertex.jointIndices.w]
                            ), 
                            rawVertex.jointWeights.w
                        );
                    }
            
                }
            }
        }
    } else {
        skinMat = IdentityMat;
    }
    
    float4x4 skinModelMat = mul(pushConsts.model, skinMat);
    
    // Position
    skinnedVertex.worldPosition = mul(skinModelMat, rawVertex.localPosition);

    // Normals
	float4 tempTangent = rawVertex.tangent;
    tempTangent = mul(skinModelMat, tempTangent);
    
    skinnedVertex.worldTangent = normalize(tempTangent.xyz);

	float4 tempNormal = float4(rawVertex.normal, 0.0);  // W is zero beacuas normal is a vector
    tempNormal = mul(skinModelMat, tempNormal);
    
    skinnedVertex.worldNormal = normalize(tempNormal.xyz);
    
	skinnedVertex.worldBiTangent = normalize(cross(skinnedVertex.worldNormal, skinnedVertex.worldTangent));

}