#ifndef CAMERA_BUFFER_HLSL
#define CAMERA_BUFFER_HLSL

struct CameraData {                                                 
    float4x4 viewProjection;                                        
    float3 cameraPosition;                                          
    float projectFarToNearDistance;
    float2 viewportDimension;
    float2 placeholder;
};                                                                  

#endif
