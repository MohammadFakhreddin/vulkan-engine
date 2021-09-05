struct VSIn {
    float3 position : POSITION0;
};

struct VSOut {
    float4 position : SV_POSITION;
};

// TOOD Just get Combined MVP
struct ModelViewProjectionBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

ConstantBuffer <ModelViewProjectionBuffer> mvpBuffer: register(b0, space0);

VSOut main(VSIn input) {
    VSOut output;

    float4x4 mvpMatrix = mul(mvpBuffer.projection, mul(mvpBuffer.view, mvpBuffer.model));
    output.position = mul(mvpMatrix, float4(input.position, 1.0));
    
    return output;
}