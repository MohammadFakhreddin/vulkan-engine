struct VSIn{
    float3 vertex_position:POSITION;
    float3 vertex_color:COLOR0;
};

struct VSOut{
    float4 position:SV_POSITION;
    float3 color:COLOR0;
};

// struct MyStruct{
//     float4x4 world;
// };

cbuffer myUniform : register (b0,space0) {
    // float a; If float exists world start from 16
    // float4x4 world;
    float4x4 model;
    float4x4 view;
    float4x4 proj;
}; // Read about alignment rules

//ConstantBuffer <MyStruct> myUniform : register (b0,space0); 
// Texture2D myTexture : register(t1,space0); // b,t,u,? 4 in general

VSOut main(VSIn input) {
    VSOut output;
    // float4x4 uniformMult = model;
    float4x4 uniformMult = mul(model,view);
    uniformMult = mul(uniformMult,proj);
    output.position = mul(uniformMult, float4(input.vertex_position,1.0f));
    // output.position = mul(model * view * proj, float4(input.vertex_position,1.0f));
    output.color = input.vertex_color;//float4(input.vertex_color, 1.0f);
    return output;
}