struct VSIn {
    float3 position : POSITION0;

    int hasSkin;
    int4 jointIndices;
    float4 jointWeights; 
};

struct VSOut {
    float4 position : SV_POSITION;
    float4 worldPosition : POSITION0;
};

struct ModelData {
    float4x4 model;
};

ConstantBuffer <ModelData> modelBuffer: register(b0, space0);

struct ViewDataProjectionData {
    float4x4 data[6];
};

ConstantBuffer <ViewDataProjectionData> viewProjectionBuffer: register(b1, space0);

struct NodeTranformation {
    float4x4 model;
};

ConstantBuffer <NodeTranformation> nodeTransformBuffer: register(b2, space0);

struct SkinJoints {
    float4x4 joints[];
};

ConstantBuffer <SkinJoints> skinJointsBuffer: register(b3, space0); 

struct PushConsts
{
	int faceIndex;
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
        skinMat = mul(skinJointsBuffer.joints[input.jointIndices.x], input.jointWeights.x)
            + mul(skinJointsBuffer.joints[input.jointIndices.y], input.jointWeights.y) 
            + mul(skinJointsBuffer.joints[input.jointIndices.z], input.jointWeights.z)
            + mul(skinJointsBuffer.joints[input.jointIndices.w], input.jointWeights.w);
    } else {
        skinMat = IdentityMat;
    }
    float4x4 modelMat = mul(modelBuffer.model, nodeTransformBuffer.model);
    float4x4 skinModelMat = mul(modelMat, skinMat);
    
    // Position
    float4 tempPosition = float4(input.position, 1.0f); // w is 1 because position is a coordinate
    float4 worldPosition = mul(skinModelMat, tempPosition);;
    output.worldPosition = worldPosition;
    output.position = mul(viewProjectionBuffer.data[pushConsts.faceIndex], worldPosition);

    return output;
}