struct VSIn {
    float3 position : POSITION0;
};

struct VSOut {
    float4 position;
};

struct ViewProjectionBuffer {
    float4x4 view;
    float4x4 projectionMat;
};

ConstantBuffer <ViewProjectionBuffer> modelTransformBuffer: register(b0, space0);

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
    
    float4 worldPos = tempPosition;
    float4 position = mul(modelTransformBuffer.projectionMat, worldPos);
    output.position = position;
    
    return output;
}