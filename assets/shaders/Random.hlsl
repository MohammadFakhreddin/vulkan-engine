#ifndef RANDOM_HLSL
#define RANDOM_HLSL

    // Simple noise function, sourced from http://answers.unity.com/answers/624136/view.html
    // Extended discussion on this function can be found at the following link:
    // https://forum.unity.com/threads/am-i-over-complicating-this-random-function.454887/#post-2949326
    // Returns a number in the 0...1 range.
    float rand(float3 seed)
    {
        return frac(sin(dot(seed.xyz, float3(12.9898, 78.233, 53.539))) * 43758.5453);
    }

    float3 rand(float3 seed, float3 minimum, float3 maximum)
    {
        return rand(seed) * (maximum - minimum) + minimum;
    }

    float2 rand(float3 seed, float2 minimum, float2 maximum)
    {
        return rand(seed) * (maximum - minimum) + minimum;
    }

    float rand(float3 seed, float minimum, float maximum)
    {
        return rand(seed) * (maximum - minimum) + minimum;
    }

#endif