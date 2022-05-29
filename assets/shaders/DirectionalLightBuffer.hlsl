#ifndef DIRECTIONAL_LIGHT_BUFFER_HLSL
#define DIRECTIONAL_LIGHT_BUFFER_HLSL

#define MAX_DIRECTIONAL_LIGHT_COUNT 3

struct DirectionalLight                                                                     
{                                                                                           
    float3 direction;                                                                       
    float placeholder0;                                                                     
    float3 color;                                                                           
    float placeholder1;                                                                     
    float4x4 viewProjectionMatrix;                                                          
};      
                                                                                    
struct DirectionalLightBufferData                                                           
{                                                                                           
    uint count;                                                                             
    uint3 placeholder;                                                                      
    DirectionalLight items [MAX_DIRECTIONAL_LIGHT_COUNT];                                   
};                                                                                          

#endif