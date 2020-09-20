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
    float4x4 rotation;
    float4x4 transformation;
    float4x4 perspective;
}; // Read about alignment rules

//ConstantBuffer <MyStruct> myUniform : register (b0,space0); 
// Texture2D myTexture : register(t1,space0); // b,t,u,? 4 in general

VSOut main(VSIn input) {

    VSOut output;
    
    float4 mult_result = mul(rotation, float4(input.vertex_position, 1.0f));

    mult_result = mul(transformation, mult_result);
    
    mult_result = mul(perspective, mult_result);

    output.position = mult_result;
    
    output.color = input.vertex_color;

    return output;
}