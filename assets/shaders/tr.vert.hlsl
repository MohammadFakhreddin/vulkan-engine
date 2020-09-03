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
    // float4x4 view;
    // float4x4 proj;
    // float4x4 camera;
}; // Read about alignment rules

//ConstantBuffer <MyStruct> myUniform : register (b0,space0); 
// Texture2D myTexture : register(t1,space0); // b,t,u,? 4 in general

VSOut main(VSIn input) {

    float1 LEFT = -320.0f;
    float1 RIGHT = 320.0f;
    float1 TOP = 240.0f;
    float1 BOTTOM = -240.0f;
    float1 ZFAR = -300.0f;
    float1 ZNEAR = 300.0f;

    VSOut output;
    // float4x4 uniformMult = model;
    // float4x4 uniformMult = mul(model, view);
    // uniformMult = mul(uniformMult, proj);
    float4 mult_result = mul(model, float4(input.vertex_position, 1.0f));
    // float4 mult_result = float4(input.vertex_position, 1.0f);
    float1 x = mult_result.x - LEFT;
    x *= 2.0f;
    x /= RIGHT - LEFT;
    x -= 1.0f;
    float1 y = mult_result.y - BOTTOM;
    y *= 2.0f;
    y /= TOP - BOTTOM;
    y -= 1.0f;
    float1 z = mult_result.z - ZFAR;
    z /= ZNEAR - ZFAR;
    output.position = float4(x,y,z,0.6f);
    // mult_result = mul(camera,mult_result);
    // output.position = mul(proj,mult_result);
    // output.position = mult_result;
    // output.position = mul(model * view * proj, float4(input.vertex_position,1.0f));
    output.color = input.vertex_color;//float4(input.vertex_color, 1.0f);
    return output;
}