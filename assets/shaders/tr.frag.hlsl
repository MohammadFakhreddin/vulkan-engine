struct VSOut{
    float4 position:SV_POSITION;
    float3 color:COLOR0;
    float2 tex_coord:TEXCOORD0;
};

struct PSOut{
    float4 color:SV_Target0;
};


sampler sampler_default : register(s1,space0);
Texture2D g_MeshTexture : register(t1,space0);

PSOut main(VSOut input) {
    PSOut output;
    // output.color = float4(input.color, 1.0);
    output.color = g_MeshTexture.Sample(sampler_default, input.tex_coord);
    return output;
}