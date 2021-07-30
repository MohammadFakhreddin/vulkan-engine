struct VSIn {
    float3 position : POSITION0;

    int hasSkin;
    int4 jointIndices;
    float4 jointWeights; 
};

struct VSOut {
    float4 position : SV_POSITION;
};

struct ModelTransformation {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

ConstantBuffer <ModelTransformation> modelTransformBuffer: register(b0, space0);

struct NodeTranformation {
    float4x4 model;
};

ConstantBuffer <NodeTranformation> nodeTransformBuffer: register(b1, space0);

struct SkinJoints {
    float4x4 joints[];
};

ConstantBuffer <SkinJoints> skinJointsBuffer: register(b2, space0); 

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
    float4x4 modelMat = mul(modelTransformBuffer.model, nodeTransformBuffer.model);
    float4x4 skinModelMat = mul(modelMat, skinMat);
    float4x4 modelViewMat = mul(modelTransformBuffer.view, skinModelMat);

    // Position
    float4 worldPosition = float4(input.position, 1.0f); // w is 1 because position is a coordinate
    worldPosition = mul(modelViewMat, worldPosition);
    output.position = worldPosition;
    
    return output;
}