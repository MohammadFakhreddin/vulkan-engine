# MFA engine :))

<br/>

More document/tutorial will be added soon!

## Ray tracing in one weekend

<img src="examples/ray-tracing-weekend.jpeg" />

<br>

## Particle system

<img src="examples/particle-sponza.gif"  height=400>

<br>

<img src="examples/fire.gif"  height=400>

<br>

<a href="./document/ParticleSystem.md">
Read more about my particle implementation here
</a>

<br>

## Character control
<img src="examples/character-control.gif">

<br>

## Shadow mapping
<img src="examples/shadow-mapping.gif">

<br>

## Animation Time!
<img src="examples/warcraft-animation.gif">

<img src="examples/mandalorian-animation.gif">

<br>

<a href="./document/Animation.md">
Read more about my animation implementation here</a>

<br>

## PBR on GLTF models
<img src="examples/pbr-car.gif"/>

<br>

## Support for rendering scene from gltf node tree
<img src="examples/gunship.gif"/>
<img src="examples/pilot-helmet.gif"/>

<br>

## Sponza scene
<img src="examples/sponza-scene.gif"/>

<br>

## Experimental android port

<img src="examples/android-port.jpg" height=300>

<br>

## Experimental ios port

<img src="examples/ios-port.jpg" height=300>

<br>

## .obj file:
<img src="examples/viking_house.gif">

<br>

## .gltf file:
<img src="examples/car.gif">

<br>

## Basic PRB
<img src="examples/pbr.gif">

<br>

## PBR on textured sphere
<img src="examples/pbr-sphere.gif">

<br>




## How to build
### Windows
- You can use following command to create the project
```
mkdir build64 || cmake -A x64 -S . -Bbuild64
```
- Or if you have node installed you can run ```npm run cmake-windows```
- Also you can use cmake tools that does everything for you (Available as extension for VS-Code)
- Find .sln files inside build64 and then run the project
<br/>

### Note: 
Currently most of recent examples won't run on mac because of no geometry shader support. I'm trying to use compute shader for animation, particle and replace all geometry shader usages with compute shader

### Macos

(Editor is not avialable yet)
- Using make
```
mkdir build64
cd build64
cmake .. -DCMAKE_BUILD_TYPE=Debug (Release) Or you can use npm run cmake-mac
make
./MFaEngine
```

- If you prefer Xcode, create a build folder then execute following command from inside that folder:
```
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=./ios.toolchain.cmake -DPLATFORM=MAC
```

<br/>

### Linux

(Editor is not avialable yet)
```
mkdir build
cd build
cmake ..

```

For debug build
```
cmake -DDEBUG_MODE=ON ..
```

For release build
```
No extra flag needed
```

https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/platformreadme/linux/readme_linux.html

You might need to install x11 library on your machine
```
sudo apt install libx11-dev
```

<br/>

### Android (Experimental)
```
Open android folder using android studio. You might need to change ndk version based on your installed version
```
<br/>

### IOS (Experimental)
```
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=./ios.toolchain.cmake -DPLATFORM=OS64COMBINED
```
<br/>

