#include "../Random.hlsl"
#include "../TimeBuffer.hlsl"

struct Params {
    int count;
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
    float3 localPosition;
    int textureIndex;

    float3 color;
    float alpha;
    
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

ConstantBuffer<Time> time : register(b2, space0);

[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    // Current SSBO index
    uint index = GlobalInvocationID.x;
	// Don't try to write beyond particle count
    if (index >= params.count)
    {
		return;
    }

    // TODO: Instead of reading from rand  we should use a noise texture
    
    float3 deltaPosition = time.deltaTime * particles[index].speed * params.moveDirection;
    particles[index].localPosition += deltaPosition;
    particles[index].localPosition += rand(particles[index].localPosition, params.noiseMin, params.noiseMax) * time.deltaTime;
    
    particles[index].remainingLifeInSec -= time.deltaTime;
    if (particles[index].remainingLifeInSec <= 0)
    {
        particles[index].localPosition = particles[index].initialLocalPosition;

        particles[index].speed = rand(particles[index].localPosition, params.minSpeed, params.maxSpeed);
        particles[index].remainingLifeInSec = rand(particles[index].localPosition, params.minLife, params.maxLife);
        particles[index].totalLifeInSec = particles[index].remainingLifeInSec;
    }

    float lifePercentage = particles[index].remainingLifeInSec / particles[index].totalLifeInSec;

    particles[index].alpha = params.alpha;

    particles[index].pointSize = params.pointSize * lifePercentage;
}

