struct PSIn {
    float4 position : SV_POSITION;
};

struct PSOut {
    float4 color : SV_Target0;
};

struct PrimitiveInfo {    // TODO Use passed color instead of primitive info
    float4 baseColorFactor : COLOR0;
};

ConstantBuffer <PrimitiveInfo> smBuff : register (b2, space0);

PSOut main(PSIn input) {
    PSOut output;

    float3 color = smBuff.baseColorFactor.rgb;
    // exposure tone mapping
    float exposure = 1.0f;
    if (color.r > exposure) {
        exposure = color.r;
    }
    if (color.g > exposure) {
        exposure = color.g;
    }
    if (color.b > exposure) {
        exposure = color.b;
    }
    exposure = 1 / exposure;

    color = float3(1.0) - exp(-color * exposure);
    // Gamma correct
    color = pow(color, float3(1.0f/2.2f)); 

    output.color = float4(color, 1.0f);
    return output;
}