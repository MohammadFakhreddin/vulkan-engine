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
    float4x4 view;
    float4x4 projectionMat;
};

ConstantBuffer <ModelTransformation> modelTransformBuffer: register(b0, space0);

struct NodeTranformation {
    float4x4 model;
};

ConstantBuffer <NodeTranformation> nodeTransformBuffer: register(b1, space0);

VSOut main(VSIn input) {
    VSOut output;

    float4x4 modelViewMat = mul(modelTransformBuffer.view, nodeTransformBuffer.model);

    // Position
    float4 tempPosition = float4(input.position, 1.0f);
    tempPosition = mul(modelViewMat, tempPosition);
    // tempPosition = mul(nodeTransformBuffer.model, tempPosition);
    // tempPosition = mul(modelTransformBuffer.view, tempPosition);
    
    float4 worldPos = tempPosition;
    float4 position = mul(modelTransformBuffer.projectionMat, worldPos);
    output.position = position;
    output.worldPos = worldPos.xyz;

    // BaseColor
    output.baseColorTexCoord = input.baseColorTexCoord;
    
    // Metallic/Roughness
    output.metallicRoughnessTexCoord = input.metallicRoughnessTexCoord;
    
    // Normals
	float4 tempTangent = input.tangent;
    tempTangent = mul(modelViewMat, tempTangent);
    // tempTangent = mul(nodeTransformBuffer.model, tempTangent);
    // tempTangent = mul(modelTransformBuffer.view, tempTangent);
    
    float3 worldTangent = normalize(tempTangent.xyz);

	float4 tempNormal = float4(input.normal, 0.0);
    tempNormal = mul(modelViewMat, tempNormal);
    // tempNormal = mul(nodeTransformBuffer.model, tempNormal);
    // tempNormal = mul(modelTransformBuffer.view, tempNormal);
    
    float3 worldNormal = normalize(tempNormal.xyz);
    
	float3 worldBiTangent = normalize(cross(worldNormal.xyz, worldTangent.xyz));
    
    output.normalTexCoord = input.normalTexCoord;
    output.worldTangent = worldTangent;
    output.worldNormal = worldNormal;
    output.worldBiTangent = worldBiTangent;

    output.emissiveTexCoord = input.emissiveTexCoord;

    return output;
}