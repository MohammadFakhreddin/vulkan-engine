# Animation

```
Note: This document is a WIP and might have issues.
```

Following resources are recommended to learn more about animations:
## LearnOpenGL.com
You get a general idea of what you need here:

<a href="https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation">
https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation
</a>

## GLTF
I used gltf documentation heavily for implementing animations.
## Sascha willems github
A good and basic example of how animations are done using GLTF format.

<a href="https://github.com/SaschaWillems/Vulkan">https://github.com/SaschaWillems/Vulkan</a>

Note: This implementation is very slow and needs optimizations. Just to get a general idea!

## Youtube
<a href="https://www.youtube.com/watch?v=f3Cr8Yx3GGA">
https://www.youtube.com/watch?v=f3Cr8Yx3GGA
</a>

Note: He explains really good but his implementation missing the inverseTransform part. Use SaschaWillems github for correctness check. 

## My code

Using mentioned resources, I implemented my version of animations. Currently animations are running multi-threaded per variant (Each instance have a separate job).
I did 2 things to optimize animations:
- Caching nodes and joint transforms improves performance significantly. Just compute animations only if parent node or attached transform is changed. 
- Using jobs per instance. (Assigning jobs to threads)

Although former optimization improved performance alot. I was not able to render 1000 model soldier with my desired fps count. I have plan to do following optimizations as well:
- SIMD
- Using compute shader for animations.
You can read general idea in wicked engine blog using this link:
<a href="https://wickedengine.net/2017/09/09/skinning-in-compute-shader/">
https://wickedengine.net/2017/09/09/skinning-in-compute-shader/
</a>