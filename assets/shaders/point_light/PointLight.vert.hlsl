struct VSIn {
    float3 position : POSITION0;
};

struct VSOut {
    float4 position : SV_POSITION;
};

struct ViewProjectionBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

ConstantBuffer <ViewProjectionBuffer> viewProjectionBuffer: register(b0, space0);

struct NodeTranformation {
    float4x4 model;
};

ConstantBuffer <NodeTranformation> nodeTransformBuffer: register(b1, space0);

VSOut main(VSIn input) {
    VSOut output;

    float4x4 modelMat = mul(viewProjectionBuffer.model, nodeTransformBuffer.model);
    float4x4 modelViewMat = mul(viewProjectionBuffer.view, modelMat);

    // Position
    float4 tempPosition = float4(input.position, 1.0f);
    tempPosition = mul(modelViewMat, tempPosition);
    
    float4 worldPos = tempPosition;
    float4 position = mul(viewProjectionBuffer.projection, worldPos);
    output.position = position;
    
    return output;
}