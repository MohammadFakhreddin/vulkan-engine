#include "../PointLightBuffer.hlsl"

struct GSInput
{
    float4 worldPosition : POSITION0;
};

struct GSOutput
{
    float4 position: SV_POSITION;
    float4 worldPosition : POSITION0;
    int Layer : SV_RenderTargetArrayIndex;
};

POINT_LIGHT(pointLightsBuffer)

struct PushConsts
{   
    int lightIndex;
    int placeholder0;
    int placeholder1;
    int placeholder2;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

#define MAX_VERTEX_COUNT 18//90 // 3 * CubeFaces

[maxvertexcount(MAX_VERTEX_COUNT)]                           // Number of generated vertices
void main(triangle GSInput input[3], /*uint InvocationID : SV_GSInstanceID,*/ inout TriangleStream<GSOutput> outStream)
{
    for(int faceIndex = 0; faceIndex < 6; ++faceIndex)
    {
        for(int i = 0; i < 3; ++i)          // for each triangle vertex
        {
            GSOutput output = (GSOutput)0;
            output.worldPosition = input[i].worldPosition;
            output.position = mul(pointLightsBuffer.items[pushConsts.lightIndex].viewProjectionMatrices[faceIndex], input[i].worldPosition);
            output.Layer = pushConsts.lightIndex * 6 + faceIndex;       // Specifies which layer of cube array we render on.
            outStream.Append(output);       // Emit vertex
        }
        outStream.RestartStrip();           // End primitive
    }   
}