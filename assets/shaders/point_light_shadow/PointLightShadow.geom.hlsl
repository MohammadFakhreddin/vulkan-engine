struct GSInput
{
    float4 worldPosition : POSITION0;
};

struct GSOutput
{
    float4 position: SV_POSITION;
    float4 worldPosition : POSITION0;
    int lightIndex;
	int Layer : SV_RenderTargetArrayIndex;
};

// Maybe we can have a separate file for this data type
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

#define MAX_POINT_LIGHT_COUNT 5

struct PointLightsBufferData
{
    uint count;
    float constantAttenuation;
    float2 placeholder;

    PointLight items [MAX_POINT_LIGHT_COUNT];                                       // Max light
};

ConstantBuffer <PointLightsBufferData> pointLightsBuffer: register(b2, space0);

struct PushConsts
{   
    float4x4 model;
    float4x4 inverseNodeTransform;
    int skinIndex;
    int placeholder0;
    int placeholder1;
    int placeholder2;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

#define MAX_VERTEX_COUNT 90 // 3 * CubeFaces * MaxPointLight

[maxvertexcount(MAX_VERTEX_COUNT)]                           // Number of generated vertices
void main(triangle GSInput input[3], /*uint InvocationID : SV_GSInstanceID,*/ inout TriangleStream<GSOutput> outStream)
{
    for (int lightIndex = 0; lightIndex < pointLightsBuffer.count; ++lightIndex)
    {
        for(int faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            for(int i = 0; i < 3; ++i)          // for each triangle vertex
            {
                GSOutput output = (GSOutput)0;
                output.worldPosition = input[i].worldPosition;
                output.position = mul(pointLightsBuffer.items[lightIndex].viewProjectionMatrices[faceIndex], input[i].worldPosition);
                output.lightIndex = lightIndex;
                output.Layer = lightIndex * 6 + faceIndex;       // Specifies which layer of cube array we render on.
                outStream.Append(output);       // Emit vertex
            }
            outStream.RestartStrip();           // End primitive
        }
    }
}