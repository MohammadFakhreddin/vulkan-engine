#ifndef CAMERA_BUFFER_HLSL
#define CAMERA_BUFFER_HLSL

struct CameraData {    
    // float projectFarToNearDistance : packoffset(c0);
    // float2 viewportDimension : packoffset(c2.y);
    // float3 cameraPosition : packoffset(c1);                                                                   
    float4x4 viewProjection :  packoffset(c0);
};                                                                  

#endif
