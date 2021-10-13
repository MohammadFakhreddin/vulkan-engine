struct VSIn {
    float3 position : POSITION0;
};

struct VSOut {
    float4 position : SV_POSITION;
};

struct ViewProjectionData {
    float4x4 viewProjection;
};

ConstantBuffer <ViewProjectionData> viewProjectionBuffer: register(b0, space0);

struct PushConsts
{   
    float4x4 model;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

VSOut main(VSIn input) {
    VSOut output;
    
    // Position
    float4 tempPosition = float4(input.position, 1.0f); // w is 1 because position is a coordinate
    float4 worldPosition = mul(pushConsts.model, tempPosition);;
    output.position = mul(viewProjectionBuffer.viewProjection, worldPosition);

    return output;
}