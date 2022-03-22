#include "../Random.hlsl"
#include "../TimeBuffer.hlsl"

struct Params {
    int vertexCount;
    float3 moveDirection;
    
    float minLife;
    float maxLife;
    float minSpeed;
    float maxSpeed;

    float radius;
    float3 noiseMin;

    float alpha;    
    float3 noiseMax;

    float pointSize;
    float3 placeholder;
};

struct Particle
{
    // Per vertex data
    float3 localPosition : SV_POSITION;
    int textureIndex;

    float3 color : COLOR0;
    float alpha : COLOR1;
    
    float pointSize;
    // State variables
    float remainingLifeInSec;
    float totalLifeInSec;
    float placeholder0;
    
    float3 initialLocalPosition;
    float speed;
};

ConstantBuffer<Params> params : register(b0, space1);
RWStructuredBuffer<Particle> particles : register(u1, space1);

ConstantBuffer<Time> time : register(b1, space0);

[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    // Current SSBO index
    uint index = GlobalInvocationID.x;
	// Don't try to write beyond vertexCount
    if (index >= params.vertexCount)
    {
		return;
    }

    Particle particle = particles[index];

    // TODO: Instead of reading from rand  we should use a noise texture
    
    float3 deltaPosition = time.deltaTime * particle.speed * params.moveDirection;
    particle.localPosition += deltaPosition;
    particle.localPosition += rand(particle.localPosition, params.noiseMin, params.noiseMax) * time.deltaTime;
    
    particle.remainingLifeInSec -= time.deltaTime;
    if (particle.remainingLifeInSec <= 0)
    {
        particle.localPosition = particle.initialLocalPosition;

        particle.speed = rand(particle.localPosition, params.minSpeed, params.maxSpeed);
        particle.remainingLifeInSec = rand(particle.localPosition, params.minLife, params.maxLife);
        particle.totalLifeInSec = particle.remainingLifeInSec;
    }

    float lifePercentage = particle.remainingLifeInSec / particle.totalLifeInSec;

    particle.alpha = params.alpha;

    particle.pointSize = params.pointSize * lifePercentage;
}

