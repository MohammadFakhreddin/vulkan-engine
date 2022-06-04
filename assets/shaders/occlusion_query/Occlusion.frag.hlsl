// #include "../PrimitiveInfoBuffer.hlsl"
// #include "../MaxTextureCount.hlsl"

// struct PSIn {
//     float4 position : SV_POSITION;   
// };

// void main(PSIn input) {
//     PrimitiveInfo primitiveInfo = primitiveInfoBuffer.primitiveInfo[pushConsts.primitiveIndex];
//     if (primitiveInfo.alphaMode == 1) {
//         float4 baseColor = primitiveInfo.baseColorTextureIndex >= 0
//             ? textures[primitiveInfo.baseColorTextureIndex].Sample(textureSampler, input.baseColorTexCoord).rgba
//             : primitiveInfo.baseColorFactor.rgba;

//         if (baseColor.a < primitiveInfo.alphaCutoff)
//         {
//             discard;
//         }
//     }
// }