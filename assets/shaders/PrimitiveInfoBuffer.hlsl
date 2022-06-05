#ifndef PRIMITIVE_INFO_BUFFER_HLSL
#define PRIMITIVE_INFO_BUFFER_HLSL

// TODO Move hasSkin to vertex
struct PrimitiveInfo {                                                  
    float4 baseColorFactor: COLOR0;                 // Color in unnecessary because we do not have interpolation for primitiveInfo

    float3 emissiveFactor: COLOR3;                                      
    float placeholder0;                                                 
                                                                        
    int baseColorTextureIndex;                                          
    float metallicFactor: COLOR1;                                       
    float roughnessFactor: COLOR2;                                      
    int metallicRoughnessTextureIndex;                                  
                                                                        
    int normalTextureIndex;                                             
    int emissiveTextureIndex;                                           
    int hasSkin;                                                        
    int occlusionTextureIndex;

    int alphaMode;
    float alphaCutoff;
    float placeholder1;
    float placeholder2;                                                   
};                                                                      
                                                                        
struct PrimitiveInfoBuffer {                                            
    PrimitiveInfo primitiveInfo[];                                      
};

#endif