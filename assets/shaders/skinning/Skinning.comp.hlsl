struct UnSkinnedVertex {
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
    uint vertexCount;
    uint vertexStartingIndex;
    int placeholder1;
};

StructuredBuffer <UnSkinnedVertex> unSkinnedVertices : register(b0, space0);
ConstantBuffer <SkinJoints> skinJoints: register(b0, space1);
RWStructuredBuffer<SkinnedVertex> skinnedVertices : register(u1, space1);

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};
// TODO: We can have a matrix class and put it there
#define IdentityMat float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

#define unSkinnedVertex unSkinnedVertices[index]
#define skinnedVertex skinnedVertices[index]

[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint index = GlobalInvocationID.x;
    if (index >= pushConsts.vertexCount)
    {
        return;
    }

    index = index + pushConsts.vertexStartingIndex;

    float4x4 skinMat = IdentityMat;
    if (unSkinnedVertex.hasSkin == 1) {
        int skinIndex = pushConsts.skinIndex;
        float4x4 inverseNodeTransform = pushConsts.inverseNodeTransform;
        if (unSkinnedVertex.jointWeights.x > 0) {
            float4x4 jointMat = 0; 
            jointMat += mul(
                skinJoints.joints[skinIndex + unSkinnedVertex.jointIndices.x],
                unSkinnedVertex.jointWeights.x
            );

            if (unSkinnedVertex.jointWeights.y > 0) {
            
                jointMat += mul(
                    skinJoints.joints[skinIndex + unSkinnedVertex.jointIndices.y],
                    unSkinnedVertex.jointWeights.y
                );
            
                if (unSkinnedVertex.jointWeights.z > 0) {
            
                    jointMat += mul( 
                        skinJoints.joints[skinIndex + unSkinnedVertex.jointIndices.z],
                        unSkinnedVertex.jointWeights.z
                    );
            
                    if (unSkinnedVertex.jointWeights.w > 0) {
                        jointMat += mul(
                            skinJoints.joints[skinIndex + unSkinnedVertex.jointIndices.w],
                            unSkinnedVertex.jointWeights.w
                        );
                    }
            
                }
            }

            skinMat = mul(inverseNodeTransform, jointMat);
        }
    } 
    
    float4x4 skinModelMat = mul(pushConsts.model, skinMat);
    
    // Position
    skinnedVertex.worldPosition = mul(skinModelMat, unSkinnedVertex.localPosition);

    // Normals
	float4 tempTangent = unSkinnedVertex.tangent;
    tempTangent = mul(skinModelMat, tempTangent);
    
    skinnedVertex.worldTangent = normalize(tempTangent.xyz);

	float4 tempNormal = float4(unSkinnedVertex.normal, 0.0);  // W is zero because normal is a vector
    tempNormal = mul(skinModelMat, tempNormal);
    
    skinnedVertex.worldNormal = normalize(tempNormal.xyz);
    
	skinnedVertex.worldBiTangent = normalize(cross(skinnedVertex.worldNormal, skinnedVertex.worldTangent));

}