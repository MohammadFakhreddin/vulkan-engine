struct PSIn {
    float4 position : SV_POSITION;
    float2 baseColorUV : TEXCOORD0;
    float2 metallicUV : TEXCOORD1;
    float2 roughnessUV : TEXCOORD2;
    float2 normalUV : TEXCOORD2;
    float3 worldNormal : NORMAL0;
    float3 worldTangent: TEXCOORD3;
    float3 worldBiTangent : TEXCOORD4;
    float3 worldPos: POSITION0;
};

struct PSOut{
    float4 color:SV_Target0;
};

sampler baseColorSampler : register(s1, space0);
Texture2D baseColorTexture : register(t1, space0);

sampler metallicSampler : register(s2, space0);
Texture2D metallicTexture : register(t2, space0);

sampler roughnessSampler : register(s3, space0);
Texture2D roughnessTexture : register(t3, space0);

sampler normalSampler : register(s4, space0);
Texture2D normalTexture : register(t4, space0);

struct LightViewBuffer {
    float3 lightPosition;
    float3 camPos;
    float3 lightColor;
};

ConstantBuffer <LightViewBuffer> lvBuff : register (b5, space0);

const float PI = 3.14159265359;

const float attenuationFactor = 200.0f;

// const float3 lightColor = float3(252.0f/256.0f, 212.0f/256.0f, 64.0f/256.0f);
// float3 lightColor = float3(1.0, 1.0, 1.0);
	
// This function computes ratio between amount of light that reflect and refracts
// Reflection contrbutes to specular while refraction contributes to diffuse light
/*
    F0 parameter which is known as the surface reflection at zero incidence or how much the surface reflects 
    if looking directly at the surface. The F0 varies per material and is tinted on metals as we find in 
    large material databases. In the PBR metallic workflow we make the simplifying assumption that most dielectric surfaces 
    look visually correct with a constant F0 of 0.04, while we do specify F0 for metallic surfaces as then given by the albedo value. 
    This translates to code as follows:
*/
// Fresnel function ----------------------------------------------------
float3 F_Schlick(float cosTheta, float metallic, float3 baseColor)
{
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor.rgb, metallic); // * material.specular
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Specular BRDF composition --------------------------------------------

float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness, float3 baseColor, float3 worldPos)
{
    // Precalculate vectors and dot products
	float3 H = normalize (V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

    float3 color = float3(0, 0, 0);

    float distance    = length(lvBuff.lightPosition - worldPos);
    float attenuation = attenuationFactor / (distance * distance);
    float3 radiance     = lvBuff.lightColor * attenuation;        
    
    // cook-torrance brdf
    float NDF = D_GGX(dotNH, roughness);        
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);  
    float3 F = F_Schlick(dotNV, metallic, baseColor);;       
    
    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    float3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular     = numerator / max(denominator, 0.001);  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    color += (kD * baseColor.rgb / PI + specular) * radiance * NdotL; 

	return color;
}

// TODO Use this to compute normal correctly
float3 calculateNormal(PSIn input)
{
	float3 tangentNormal = normalTexture.Sample(normalSampler, input.normalUV).rgb * 2.0 - 1.0;

	float3x3 TBN = float3x3(input.worldTangent, input.worldBiTangent, input.worldNormal);
	float3 pixelNormal = mul(TBN, tangentNormal);
	
	return normalize(pixelNormal.xyz);
}

PSOut main(PSIn input) {
	float3 baseColor = pow(baseColorTexture.Sample(baseColorSampler, input.baseColorUV).rgb, 2.2f);
    float metallic = metallicTexture.Sample(metallicSampler, input.metallicUV).r;
    float roughness = roughnessTexture.Sample(roughnessSampler, input.roughnessUV).r;
	float3 normal = input.worldNormal;//calculateNormal(input);
    // PSOut output2;
    // output2.color = float4(normal.xyz, 1.0f);
    // return output2;
	
    float3 N = normalize(normal.xyz);
	float3 V = normalize(lvBuff.camPos - input.worldPos);
    
    // Specular contribution
	float3 Lo = float3(0.0, 0.0, 0.0);
	for (int i = 0; i < 1; i++) {   // Light count
		float3 L = normalize(lvBuff.lightPosition.xyz - input.worldPos);
		Lo += BRDF(L, V, N, metallic, roughness, baseColor, input.worldPos);
	};

	// Combine with ambient
	float3 color = baseColor * 0.01; // TODO Add emissive color here
	color += Lo;

	// Gamma correct
    color = color / (color + float3(1.0f));
    color = pow(color, float3(1.0f/2.2f)); 

    PSOut output;
    output.color = float4(color, 1.0);
	return output;
}
