struct PSIn {
    float4 position : SV_POSITION;
    int textureIndex;
    float2 uv : TEXCOORD0;
    float3 color : COLOR0;
    float alpha : COLOR1;
};

struct PSOut {
    float4 color:SV_Target0;
};

#define MAX_POSSIBLE_TEXTURE 5

sampler baseColorSampler : register(s1, space0);
Texture2D baseColorTexture[MAX_POSSIBLE_TEXTURE] : register(t0, space1);

PSOut main(PSIn input) {
    PSOut output;
    output.color = input.textureIndex >= 0 
        ? baseColorTexture[input.textureIndex].Sample(baseColorSampler, input.uv)
        : float4(input.color, input.alpha);
    return output;
}