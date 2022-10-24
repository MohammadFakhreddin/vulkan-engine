#include "../TimeBuffer.hlsl"

struct Particle {       // Currently number of particles and vertices are equal
    float3 position;
    float3 velocity;
    float2 uv;
    float2 normal;
    int pinned;
};

struct PhysicalProperties {
    float particleMass;
	float springStiffness;
	float damping;
	float restLength;    // Rest length horizontal
	
    // float restLenV;    // Rest length vertical
	// float restLenD;    // Rest length diameter
    // float2 placeholder0;
    
    float3 gravity;     // Gravity
	float placeholder1;
    
    int2 particleCount; // Number of cloth partics (In our case vertices)
    float2 placeholder2;
};

struct PushConstants {
    float4x4 model;
};

[[vk::push_constant]]
cbuffer {
    PushConstants pushConsts;
};

ConstantBuffer <PhysicalProperties> properties: register(b0, space0);
StructuredBuffer<Particle> inParticles : register(u1, space0);
RWStructuredBuffer<Particle> outParticles : register(u2, space0);
ConstantBuffer <Time> time : register(b3, space0);

int ComputeIndex(int x, int y)
{
    return y * properties.particleCount.x + x;
}

Particle GetInParticle(int x, int y)
{
    int index = ComputeIndex(x, y);
    return inParticles[index];
}

Particle GetOutParticle(int x, int y) 
{
    int index = ComputeIndex(x, y);
    return outParticles[index];
}

float3 SpringForce(Particle myParticle, Particle otherParticle, float restLen) 
{
    float3 springVec = otherParticle.position - myParticle.position;
    float springLen = length(springVec);
    float strain = (springLen / restLen) - 1;
    float3 springDir = springVec / springLen;

    float3 velVec = otherParticle.position - myParticle.position;

    // f = K(x0 - x1)
    float3 springForce = springDir * strain * properties.springStiffness;
    float3 dampingForce = (dot(springDir, velVec) / restLen) * properties.damping * springDir;
    return springForce + dampingForce;
}

[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    float2 id2d = GlobalInvocationID.xy;
    
    if (id2d.x >= properties.particleCount.x || 
        id2d.y >= properties.particleCount.y) 
    {
        return;
    }
    
    Particle myInParticle = GetInParticle(id2d.x, id2d.y);
    Particle myOutParticle = GetOutParticle(id2d.x, id2d.y);

    if (myInParticle.pinned)
    {
        return;
    }

    float3 force = properties.gravity * properties.particleMass;

    // Left
    if (id2d.x > 0) {
        force += SpringForce(
            myInParticle, 
            GetInParticle(id2d.x - 1, id2d.y),
            properties.restLength 
        );
    }
    // Right
    if (id2d.x < properties.particleCount.x - 1) {
        force += SpringForce(
            myInParticle, 
            GetInParticle(id2d.x + 1, id2d.y),
            properties.restLength 
        );
    }
    // Upper right
    if (id2d.x < properties.particleCount.x - 1 && id2d.y > 0) {
        force += SpringForce(
            myInParticle, 
            GetInParticle(id2d.x + 1, id2d.y - 1),
            properties.restLength 
        );
    }
    // Upper left
    if (id2d.x > 0 && id2d.y > 0) {
        force += SpringForce(
            myInParticle, 
            GetInParticle(id2d.x - 1, id2d.y - 1),
            properties.restLength 
        );
    }
    // Up
    if (id2d.y > 0)
    {
        force += SpringForce(
            myInParticle, 
            GetInParticle(id2d.x, id2d.y - 1),
            properties.restLength 
        );
    }
    // Bottom
    if (id2d.y < properties.particleCount.y - 1)
    {
        force += SpringForce(
            myInParticle,
            GetInParticle(id2d.x, id2d.y + 1),
            properties.restLength
        );
    }
    // Bottom right
    if (id2d.y < properties.particleCount.y - 1 && id2d.x < properties.particleCount.x - 1)
    {
        force += SpringForce(
            myInParticle,
            GetInParticle(id2d.x + 1, id2d.y + 1),
            properties.restLength
        );
    }
    // Bottom left
    if (id2d.y < properties.particleCount.y - 1 && id2d.x > 0)
    {
        force += SpringForce(
            myInParticle,
            GetInParticle(id2d.x - 1, id2d.y + 1),
            properties.restLength
        );
    }
    
    float3 deltaVel = (force / properties.particleMass);
    float3 velocity = deltaVel + myInParticle.velocity;
    float3 position = velocity * time.deltaTime;

    myOutParticle.position = position;
    myOutParticle.velocity = velocity;

    // TODO: This object is double sided what should I do ?

    float3 normal = float3(0.0f, 0.0f, 0.0f);
    if (id2d.y > 0)
    {
        if (id2d.x > 0) 
        {

        }
        if (id2d.x < properties.particleCount.x - 1)
        {

        }
    }
    if (id2d.y < properties.particleCount.y - 1)
    {
        if (id2d.x > 0)
        {

        }
        if (id2d.x < properties.particleCount.x - 1)
        {

        }
    }
    
    myOutParticle.normal = normal;
}