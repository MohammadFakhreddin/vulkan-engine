# Vulkan engine :))
### Features preview and some document coming soon!

<br/>

## How to build
### Windows
```
Create directory build64
cmake . -BBuild64 -DCMAKE_GENERATOR=x64
Or 86 if you prefer (ASan does not work on x64 currently)
cmake .. -G "Visual Studio 16 2019" -A Win32
```
Find .sln files inside build64 and then run the project
<br/>

### Macos
```
mkdir build64
cd build64
cmake ..
make
./MFaEngine
```
<br/>

### Linux
Not supported yet!
<br/>

### Android (Experimental)
```
Open android folder using android studio. You might need to change ndk version based on your installed version
```
<br/>

### IOS
Not supported yet!
<br/>

## Animation Time!
<img src="examples/warcraft-animation.gif">

<img src="examples/mandalorian-animation.gif">

<br>

## Experimental android port

<img src="examples/android-port.jpg">

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

## PBR on GLTF models
<img src="examples/pbr-car.gif"/>

<br>

## Support for rendering scene from gltf node tree
<img src="examples/gunship.gif"/>
<img src="examples/pilot-helmet.gif"/>

<br>

## Sponza scene
<img src="examples/sponza-scene.gif"/>