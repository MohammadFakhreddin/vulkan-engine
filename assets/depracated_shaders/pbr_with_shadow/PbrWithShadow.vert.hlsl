struct VSIn {
    float3 position : POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;  
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    float4 tangent: TEXCOORD3;
    float3 normal: NORMAL0;

    float2 emissiveTexCoord: TEXCOORD4;

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

    float2 emissiveTexCoord: TEXCOORD4;
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
    float4 tempPosition = float4(input.position, 1.0f); // w is 1 because position is a coordinate
    tempPosition = mul(modelViewMat, tempPosition);
    
    float4 position = mul(modelTransformBuffer.projection, tempPosition);
    output.position = position;
    
    float4 tempPosition2 = float4(input.position, 1.0f); // w is 1 because position is a coordinate
    output.worldPos = mul(skinModelMat, tempPosition2).xyz;

    // BaseColor
    output.baseColorTexCoord = input.baseColorTexCoord;
    
    // Metallic/Roughness
    output.metallicRoughnessTexCoord = input.metallicRoughnessTexCoord;
    
    // Normals
	float4 tempTangent = input.tangent;
    // tempTangent = mul(modelViewMat, tempTangent);
    tempTangent = mul(skinModelMat, tempTangent);
    
    float3 worldTangent = normalize(tempTangent.xyz);

	float4 tempNormal = float4(input.normal, 0.0);  // W is zero beacuas normal is a vector
    // tempNormal = mul(modelViewMat, tempNormal);
    tempNormal = mul(skinModelMat, tempNormal);
    
    float3 worldNormal = normalize(tempNormal.xyz);
    
	float3 worldBiTangent = normalize(cross(worldNormal.xyz, worldTangent.xyz));
    
    output.normalTexCoord = input.normalTexCoord;
    output.worldTangent = worldTangent;
    output.worldNormal = worldNormal;
    output.worldBiTangent = worldBiTangent;

    output.emissiveTexCoord = input.emissiveTexCoord;

    return output;
}