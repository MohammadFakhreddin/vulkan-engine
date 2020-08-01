struct VSIn{
    float3 vertex_position:POSITION;
    float3 vertex_color:COLOR0;
};

struct VSOut{
    float4 position:SV_POSITION;
    float3 color:COLOR0;
};

VSOut main(VSIn input) {
    VSOut output;
    output.position = float4(input.vertex_position,1.0f);
    output.color = input.vertex_color;//float4(input.vertex_color, 1.0f);
    return output;
}