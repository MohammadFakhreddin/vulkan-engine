# Particle system

```
Note: This document is a WIP and might have issues.
```
 
- Particles are rendered after everything on render target with depth write disabled so behind fragments won't be discarded.
- I used additive operator so each fragment contribute to the color
- Movement of particles are updated inside compute shader (See the particle.comp)
- Due to particles being points (Maybe I could have used 2 triangle instead) we have to compute the uv position on the texture based on distance from center of point and the point size on screen. (See the particle.vert and particle.frag)

```
output.centerPosition = ((position.xy / position.w) + 1.0) * 
    0.5 * cameraBuffer.viewportDimension; // Vertex position in screen space
    
float pointSize = input.pointSize / position.w;
output.pointRadius = pointSize / 2.0f;
```