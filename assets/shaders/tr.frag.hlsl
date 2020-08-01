struct PSOut{
    float4 color:SV_Target0;
};

struct VSOut{
    float4 position:SV_POSITION;
    float3 color:COLOR0;
};

PSOut main(VSOut input) {
    PSOut output;
    output.color = float4(input.color, 1.0);
    return output;
}