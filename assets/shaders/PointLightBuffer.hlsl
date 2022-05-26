#ifndef POINT_LIGHT_BUFFER_HLSL
#define POINT_LIGHT_BUFFER_HLSL

#define MAX_POINT_LIGHT_COUNT 10

struct PointLight                                                                       
{                                                                                       
    float3 position;                                                                    
    float placeholder0;                                                                 
    float3 color;                                                                       
    float maxSquareDistance;                                                            
    float linearAttenuation;                                                            
    float quadraticAttenuation;                                                         
    float2 placeholder1;                                                                
    float4x4 viewProjectionMatrices[6];                                                 
};                                                                                      
                                                                                        
struct PointLightsBufferData                                                            
{                                                                                       
    uint count;                                                                         
    float constantAttenuation;                                                          
    float2 placeholder;                                                                 
                                                                                        
    PointLight items [MAX_POINT_LIGHT_COUNT];                                           
};                                                                                      
    
// TODO: Macro is not a good idea here
#define POINT_LIGHT(bufferName)                                                         \
ConstantBuffer <PointLightsBufferData> bufferName: register(b2, space0);                \

#endif