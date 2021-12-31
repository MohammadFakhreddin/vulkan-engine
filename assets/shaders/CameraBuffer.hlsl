#ifndef CAMERA_BUFFER_HLSL
#define CAMERA_BUFFER_HLSL

struct CameraData {                                                 
    float4x4 viewProjection;                                        
    float3 cameraPosition;                                          
    float projectFarToNearDistance;                                 
};                                                                  

#define CAMERA_BUFFER(bufferName)                                   \
ConstantBuffer <CameraData> bufferName: register(b0, space0);       \

#define CAMERA_BUFFER_CUSTOM_BINDING(bufferName, binding, space)          \
ConstantBuffer <CameraData> bufferName: register(binding, space);   \

#endif
