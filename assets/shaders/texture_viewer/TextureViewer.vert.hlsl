struct VSIn {
    float3 position : POSITION0;
    float2 uv : TEXCOORD0;
};

struct VSOut {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct ViewProjectionBuffer {
    float4x4 viewMat;
    float4x4 projectionMat;
};

ConstantBuffer <ViewProjectionBuffer> vpBuff: register(b0, space0);

VSOut main(VSIn input) {
    VSOut output;
    output.position = mul(vpBuff.projectionMat, mul(vpBuff.viewMat, float4(input.position, 1.0f)));
    output.uv = input.uv;
    return output;
}