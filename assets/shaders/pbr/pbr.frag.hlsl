// Based on https://learnopengl.com/PBR/Lighting

struct PSIn {
    float3 WorldPos: POSITION;
    float3 WorldNormal: NORMAL0;
    float4 Position: SV_POSITION;
};

struct PSOut {
    float4 FragColor: SV_Target0;
};

struct MaterialBuffer {
    float3 albedo;          // BaseColor
    float metallic;
    float roughness;
    float ao;               // Emission  
};

ConstantBuffer <MaterialBuffer> matBuff : register (b1, space0);

struct LightViewBuffer {
    float3 camPos;
    int lightCount;
    float3 lightPositions[8];
    float3 lightColors[8];
};

ConstantBuffer <LightViewBuffer> lvBuff : register (b2, space0);

const float PI = 3.14159265359;

// This function computes ratio between amount of light that reflect and refracts
// Reflection contrbutes to specular while refraction contributes to diffuse light
/*
    F0 parameter which is known as the surface reflection at zero incidence or how much the surface reflects 
    if looking directly at the surface. The F0 varies per material and is tinted on metals as we find in 
    large material databases. In the PBR metallic workflow we make the simplifying assumption that most dielectric surfaces 
    look visually correct with a constant F0 of 0.04, while we do specify F0 for metallic surfaces as then given by the albedo value. 
    This translates to code as follows:
*/
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
};

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
};

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
};

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
};

PSOut main(PSIn input) {
    PSOut tempOutput;
    tempOutput.FragColor = float4(1, 0, 0, 1.0);
    return tempOutput;
    // float3 N = normalize(input.WorldNormal);
    // float3 V = normalize(lvBuff.camPos - input.WorldPos);
    
    // // TODO We can also ask for F0 value from uniform buffer
    // float3 F0 = float3(0.04); 
    // // TODO Check if parameters have correct order
    // F0 = lerp(F0, matBuff.albedo, matBuff.metallic);

    // // Direct lighting
    // float3 Lo = float3(0.0);
    // for (int i = 0; i < lvBuff.lightCount; ++i) {
    //     float3 L = normalize(lvBuff.lightPositions[i] - input.WorldPos);
    //     float H = normalize(V + L);

    //     float3 distance = length(lvBuff.lightPositions[i] - input.WorldPos);
    //     float attenuation = 1.0 / (distance * distance);
    //     // Radiance is amount of diffuse light contribution to final result
    //     float3 radiance = lvBuff.lightColors[i] * attenuation;

    //     // cook-torrance brdf
    //     float NDF = DistributionGGX(N, H, matBuff.roughness);        
    //     float G   = GeometrySmith(N, V, L, matBuff.roughness);      
    //     float3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
    //     float3 kS = F;
    //     float3 kD = float3(1.0) - kS;
    //     kD *= 1.0 - matBuff.metallic;	  
        
    //     float3 numerator    = NDF * G * F;
    //     float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    //     float3 specular     = numerator / max(denominator, 0.001);  
            
    //     // add to outgoing radiance Lo
    //     float NdotL = max(dot(N, L), 0.0);                
    //     Lo += (kD * matBuff.albedo / PI + specular) * radiance * NdotL; 
    // }

    // float3 ambient = float3(0.03) * matBuff.albedo * matBuff.ao;
    // float3 color = ambient + Lo;
	
    // color = color / (color + float3(1.0));
    // color = pow(color, float3(1.0/2.2));  
   
    // PSOut output;

    // output.FragColor = float4(color, 1.0);

    // return output;
};