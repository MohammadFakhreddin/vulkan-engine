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
    float4x4 rotation;
    float4x4 translate;
    float4x4 projection;
};

ConstantBuffer <ModelTransformation> modelTransformBuffer: register(b0, space0);

struct NodeTranformation {
    float4x4 rotationAndScaleMat;
    float4x4 transformMat;
};

#define NODE_TREE_SUPPORT

ConstantBuffer <NodeTranformation> nodeTransformBuffer: register(b1, space0);

VSOut main(VSIn input) {
    VSOut output;
    
    // Position
#ifndef NODE_TREE_SUPPORT
    float4 rotationResult = mul(modelTransformBuffer.rotation, float4(input.position, 1.0f));
    float4 worldPos = mul(modelTransformBuffer.translate, rotationResult);
    output.position = mul(modelTransformBuffer.projection, worldPos);
    output.worldPos = worldPos.xyz;
#else 
    float4 nodePosition = mul(modelTransformBuffer.rotation, float4(input.position, 1.0f));
    float4 rotationResult = mul(nodeTransformBuffer.transformMat, nodePosition);
    float4 worldPos = mul(modelTransformBuffer.translate, rotationResult);
    output.worldPos = worldPos;
    output.position = mul(modelTransformBuffer.projection, worldPos);
#endif

    // BaseColor
    output.baseColorTexCoord = input.baseColorTexCoord;
    
    // Metallic/Roughness
    output.metallicRoughnessTexCoord = input.metallicRoughnessTexCoord;
    
#ifndef NODE_TREE_SUPPORT
    float4x4 rotationAndScaleMat = nodeTransformBuffer.rotationAndScaleMat;
#else
    float4x4 rotationAndScaleMat = mul(
        nodeTransformBuffer.rotationAndScaleMat, 
        modelTransformBuffer.rotation
    );
#endif
    // Normals
	float4 rotationTangent = mul(rotationAndScaleMat, input.tangent);
    float3 worldTangent = normalize(mul(modelTransformBuffer.translate, rotationTangent).xyz);

	float4 rotationNormal = mul(rotationAndScaleMat, float4(input.normal, 0.0));
    float3 worldNormal = normalize(mul(modelTransformBuffer.translate, rotationNormal).xyz);
    
	float3 worldBiTangent = normalize(cross(worldNormal.xyz, worldTangent.xyz));
    
    output.normalTexCoord = input.normalTexCoord;
    output.worldTangent = worldTangent;
    output.worldNormal = worldNormal;
    output.worldBiTangent = worldBiTangent;

    output.emissiveTexCoord = input.emissiveTexCoord;

    return output;
}