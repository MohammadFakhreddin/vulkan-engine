struct VSIn {
    float3 position : POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;  
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    float4 tangent: TEXCOORD3;
    float3 normal: NORMAL0;

    float2 emissiveTexCoord: TEXCOORD4;
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
    float4x4 rotationAndScaleMat;
    float4x4 translateMat;
    float4x4 projectionMat;
};

ConstantBuffer <ModelTransformation> modelTransformBuffer: register(b0, space0);

struct NodeTranformation {
    float4x4 rotationAndScaleMat;
    float4x4 translateMat;
    // float4x4 transform;
};

// #define NODE_TREE_SUPPORT

ConstantBuffer <NodeTranformation> nodeTransformBuffer: register(b1, space0);

VSOut main(VSIn input) {
    VSOut output;
    
// #ifndef NODE_TREE_SUPPORT
//     // float4x4 rotationAndScaleMat = modelTransformBuffer.rotationAndScaleMat;
//     // float4x4 translateMat = modelTransformBuffer.translateMat;

//     // Position
//     float4 rotationResult = mul(rotationAndScaleMat, float4(input.position, 1.0f));
//     float4 worldPos = mul(translateMat, rotationResult);
//     output.position = mul(modelTransformBuffer.projectionMat, worldPos);
//     output.worldPos = worldPos.xyz;
// #else
    // float4x4 rotationAndScaleMat = mul(
    //     nodeTransformBuffer.rotationAndScaleMat, 
    //     modelTransformBuffer.rotationAndScaleMat
    // );
    // float4x4 translateMat = mul(
    //     nodeTransformBuffer.translateMat,
    //     modelTransformBuffer.translateMat
    // );

    // Position
    float4 tempPosition = float4(input.position, 1.0f);
    // tempPosition = mul(nodeTransformBuffer.transform, tempPosition);
    tempPosition = mul(nodeTransformBuffer.translateMat, tempPosition);
    tempPosition = mul(nodeTransformBuffer.rotationAndScaleMat, tempPosition);
    tempPosition = mul(modelTransformBuffer.rotationAndScaleMat, tempPosition);
    tempPosition = mul(modelTransformBuffer.translateMat, tempPosition);
    float4 worldPos = tempPosition;
    output.position = mul(modelTransformBuffer.projectionMat, worldPos);
    output.worldPos = worldPos.xyz;
// #endif

    // BaseColor
    output.baseColorTexCoord = input.baseColorTexCoord;
    
    // Metallic/Roughness
    output.metallicRoughnessTexCoord = input.metallicRoughnessTexCoord;
    
    // TODO: Refactor and create function
    // Normals
	float4 tempTangent = input.tangent;
    // tempTangent = mul(nodeTransformBuffer.transform, tempTangent);
    tempTangent = mul(nodeTransformBuffer.translateMat, tempTangent);
    tempTangent = mul(nodeTransformBuffer.rotationAndScaleMat, tempTangent);
    tempTangent = mul(modelTransformBuffer.rotationAndScaleMat, tempTangent);
    tempTangent = mul(modelTransformBuffer.translateMat, tempTangent);

    float3 worldTangent = normalize(tempTangent.xyz);

	// float4 tempNormal = mul(rotationAndScaleMat, float4(input.normal, 0.0));
    // float3 worldNormal = normalize(mul(translateMat, tempNormal).xyz);
    float4 tempNormal = float4(input.normal, 0.0);
    // tempNormal = mul(nodeTransformBuffer.transform, tempNormal);
    tempNormal = mul(nodeTransformBuffer.translateMat, tempNormal);
    tempNormal = mul(nodeTransformBuffer.rotationAndScaleMat, tempNormal);
    tempNormal = mul(modelTransformBuffer.rotationAndScaleMat, tempNormal);
    tempNormal = mul(modelTransformBuffer.translateMat, tempNormal);

    float3 worldNormal = normalize(tempNormal.xyz);
    
	float3 worldBiTangent = normalize(cross(worldNormal.xyz, worldTangent.xyz));
    
    output.normalTexCoord = input.normalTexCoord;
    output.worldTangent = worldTangent;
    output.worldNormal = worldNormal;
    output.worldBiTangent = worldBiTangent;

    output.emissiveTexCoord = input.emissiveTexCoord;

    return output;
}