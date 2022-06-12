#ifndef PRIMITIVE_INFO_BUFFER_HLSL
#define PRIMITIVE_INFO_BUFFER_HLSL

// TODO Move hasSkin to vertex
struct PrimitiveInfo {                                                  
    float4 baseColorFactor: COLOR0 : packoffset(c0);                 // Color in unnecessary because we do not have interpolation for primitiveInfo

    float3 emissiveFactor: COLOR3 : packoffset(c1);                                      
    float placeholder0 : packoffset(c1.w);                                                 
                                                                        
    int baseColorTextureIndex : packoffset(c2);                                          
    float metallicFactor: COLOR1 : packoffset(c2.y);                                       
    float roughnessFactor: COLOR2 : packoffset(c2.z);                                      
    int metallicRoughnessTextureIndex : packoffset(c2.w);                                  
                                                                        
    int normalTextureIndex : packoffset(c3);                                             
    int emissiveTextureIndex : packoffset(c3.y);                                           
    int hasSkin : packoffset(c3.z);                                                        
    int occlusionTextureIndex : packoffset(c3.w);

    int alphaMode : packoffset(c4);
    float alphaCutoff : packoffset(c4.y);
    float placeholder1 : packoffset(c4.z);
    float placeholder2 : packoffset(c4.w);                                                   
};                                                                      
                                                                        
struct PrimitiveInfoBuffer {                                            
    PrimitiveInfo primitiveInfo[];                                      
};

#endif