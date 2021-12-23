#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"

struct VSIn {
    float3 position : POSITION0;

    int hasSkin;
    int4 jointIndices;
    float4 jointWeights; 
};

struct VSOut {
    float4 worldPosition : POSITION0;
};

SKIN_JOINTS_BUFFER(skinJointsBuffer)
CAMERA_BUFFER(cameraBuffer)

struct PushConsts
{   
    float4x4 model;
    float4x4 inverseNodeTransform;
    int skinIndex;
    int lightIndex;
    int placeholder1;
    int placeholder2;
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
    // float4x4 modelMat = mul(modelBuffer.model, nodeTransformBuffer.model);
    // float4x4 skinModelMat = mul(modelMat, skinMat);
    float4x4 skinModelMat = mul(pushConsts.model, skinMat);
    
    // Position
    float4 tempPosition = float4(input.position, 1.0f); // w is 1 because position is a coordinate
    float4 worldPosition = mul(skinModelMat, tempPosition);;
    output.worldPosition = worldPosition;
    return output;
}