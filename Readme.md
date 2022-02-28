# MFA engine :))

## Animation explanation
<a href="./document/Animation.md">Read more about my animation implementation here</a>

<br/>

More document/tutorial will be added soon!

## Particle system
<img src="examples/particle-sponza.gif"  height=400>

<br>

<img src="examples/fire.gif"  height=400>

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

<img src="examples/android-port.jpg" height=400>

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

### Macos
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
Not supported yet!
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

