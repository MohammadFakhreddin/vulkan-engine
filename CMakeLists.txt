cmake_minimum_required (VERSION 3.10)

set(CMAKE_OSX_DEPLOYMENT_TARGET 13)
set(CMAKE_IOS_DEPLOYMENT_TARGET 13)

set(PROJECT_NAME "MFaEngine")

set(UNIT_TEST_NAME "UnitTests")

project ("MFaEngine" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

if (ANDROID)
set (CMAKE_CXX_STANDARD 17)
else ()
set (CMAKE_CXX_STANDARD 20)
endif ()
set (CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
   if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(MACOSX TRUE)
   else()
        set(IPHONE TRUE)
   endif()
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS TRUE)
    # SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")# %MyVS%/lib/x64/clang_rt.asan_dynamic-x86_64.lib %MyVS%/lib/x64/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib")
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
   set(LINUX TRUE)
endif()

if(WINDOWS)
    add_definitions (/WX)           # Warnings as errors
    add_definitions (-D_CRT_SECURE_NO_WARNINGS)
else()
    # TODO
    # add_definitions(-Werror)
endif()

message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

if (NOT WINDOWS) 
    if(${CMAKE_BUILD_TYPE} MATCHES Release)
        add_definitions(-DNDEBUG)
    else()
        add_definitions(-D_DEBUG)
    endif()
endif()

include_directories(libs/include/glm)

# TODO: Linux On MacOS 10.14 with clang++ compiler, I get executable with SIMD right away.
# On Ubuntu 16.04 with g++ compiler, I will have to use the compiler flag -mavx2 to generate a SIMD executable. There are other compiler options for other SIMD instructions here. Without the compiler flag, I still get a serial executable.
add_definitions(-DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES)
add_definitions(-DGLM_FORCE_INTRINSICS)
if (LINUX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
endif ()
#-----------------------------------------------------------------------
# Engine
#-----------------------------------------------------------------------

set(ENGINE_SOURCES)

list(
    APPEND ENGINE_SOURCES

    # Bedrock
    "src/engine/BedrockAssert.cpp"
    "src/engine/BedrockAssert.hpp"
    "src/engine/BedrockCommon.hpp"
    "src/engine/BedrockFileSystem.hpp"
    "src/engine/BedrockFileSystem.cpp"
    "src/engine/BedrockLog.hpp"
    "src/engine/BedrockMath.hpp"
    "src/engine/BedrockMatrix.hpp"
    "src/engine/BedrockMatrix.cpp"
    "src/engine/BedrockMemory.hpp"
    "src/engine/BedrockMemory.cpp"
    "src/engine/BedrockPlatforms.hpp"
    "src/engine/BedrockPlatforms.cpp"
    "src/engine/BedrockPath.hpp"
    "src/engine/BedrockPath.cpp"
    "src/engine/BedrockSignal.hpp"

    # Entity system
    "src/engine/entity_system/EntitySystem.hpp"
    "src/engine/entity_system/EntitySystem.cpp"
    "src/engine/entity_system/EntityHandle.hpp"
    "src/engine/entity_system/Entity.hpp"
    "src/engine/entity_system/Entity.cpp"
    "src/engine/entity_system/Component.hpp"
    "src/engine/entity_system/Component.cpp"

    # Components
    "src/engine/entity_system/components/TransformComponent.hpp"
    "src/engine/entity_system/components/TransformComponent.cpp"
    "src/engine/entity_system/components/ColliderComponent.hpp"
    "src/engine/entity_system/components/ColliderComponent.cpp"

    "src/engine/entity_system/components/MeshRendererComponent.hpp"
    "src/engine/entity_system/components/MeshRendererComponent.cpp"
    "src/engine/entity_system/components/BoundingVolumeRendererComponent.hpp"
    "src/engine/entity_system/components/BoundingVolumeRendererComponent.cpp"
    
    "src/engine/entity_system/components/BoundingVolumeComponent.hpp"
    "src/engine/entity_system/components/BoundingVolumeComponent.cpp"
    "src/engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
    "src/engine/entity_system/components/AxisAlignedBoundingBoxComponent.cpp"
    "src/engine/entity_system/components/SphereBoundingVolumeComponent.hpp"
    "src/engine/entity_system/components/SphereBoundingVolumeComponent.cpp"

    "src/engine/entity_system/components/ColorComponent.hpp"
    

    # Job system
    "src/engine/job_system/ThreadPool.hpp"
    "src/engine/job_system/ThreadPool.cpp"
    "src/engine/job_system/ThreadSafeQueue.hpp"
    "src/engine/job_system/JobSystem.hpp"
    "src/engine/job_system/JobSystem.cpp"
    
    # RenderSystem
    "src/engine/render_system/RenderTypes.hpp"
    "src/engine/render_system/RenderTypesFWD.hpp"
    "src/engine/render_system/RenderTypes.cpp"
    "src/engine/render_system/drawable_essence/DrawableEssence.hpp"
    "src/engine/render_system/drawable_essence/DrawableEssence.cpp"
    "src/engine/render_system/drawable_variant/DrawableVariant.hpp"
    "src/engine/render_system/drawable_variant/DrawableVariant.cpp"
    "src/engine/render_system/RenderBackend.hpp"
    "src/engine/render_system/RenderBackend.cpp"
    "src/engine/render_system/RenderFrontend.hpp"
    "src/engine/render_system/RenderFrontend.cpp"

    # RenderPass
    "src/engine/render_system/render_passes/RenderPass.hpp"
    "src/engine/render_system/render_passes/RenderPass.cpp"
    "src/engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
    "src/engine/render_system/render_passes/display_render_pass/DisplayRenderPass.cpp"
    "src/engine/render_system/render_passes/shadow_render_pass_v2/ShadowRenderPassV2.hpp"
    "src/engine/render_system/render_passes/shadow_render_pass_v2/ShadowRenderPassV2.cpp"
    
    # Pipeline
    "src/engine/render_system/pipelines/BasePipeline.hpp"
    "src/engine/render_system/pipelines/BasePipeline.cpp"
    "src/engine/render_system/pipelines/DescriptorSetSchema.hpp"
    "src/engine/render_system/pipelines/DescriptorSetSchema.cpp"

    "src/engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
    "src/engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.cpp"
    
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.cpp"

    # AssetSystem
    "src/engine/FoundationAsset.hpp"
    "src/engine/FoundationAsset.cpp"
    
    # UI System
    "src/engine/ui_system/UISystem.hpp"
    "src/engine/ui_system/UISystem.cpp"

    # Camera
    "src/engine/camera/CameraComponent.hpp"
    "src/engine/camera/CameraComponent.cpp"
    "src/engine/camera/ObserverCameraComponent.hpp"
    "src/engine/camera/ObserverCameraComponent.cpp"
    "src/engine/camera/ThirdPersonCameraComponent.hpp"
    "src/engine/camera/ThirdPersonCameraComponent.cpp"

    # InputManager
    "src/engine/InputManager.hpp"
    "src/engine/InputManager.cpp"

    # SceneManager
    "src/engine/scene_manager/SceneManager.hpp"
    "src/engine/scene_manager/SceneManager.cpp"
    "src/engine/scene_manager/Scene.hpp"
    "src/engine/scene_manager/Scene.cpp"
    
)

add_library("Engine"
    ${ENGINE_SOURCES}
)

target_include_directories(
    "Engine" PUBLIC 
    "src" 
    "${CMAKE_SOURCE_DIR}/libs/include"
)

#-----------------------------------------------------------------------
# Tools
#-----------------------------------------------------------------------

set(TOOL_SOURCES)

# file(GLOB_RECURSE TOOLS_HEADER_H "src/tools/*.h")
# file(GLOB_RECURSE TOOLS_HEADER_HPP "src/tools/*.hpp")
# file(GLOB_RECURSE TOOLS_CPP "src/tools/*.cpp")

list(
    APPEND TOOLS_SOURCES
    "src/tools/ImageUtils.cpp"
    "src/tools/ImageUtils.hpp"
    "src/tools/Importer.cpp"
    "src/tools/Importer.hpp"
    "src/tools/ShapeGenerator.cpp"
    "src/tools/ShapeGenerator.hpp"
)

add_library("Tools"
    ${TOOLS_SOURCES}
)

target_include_directories(
    "Tools" PUBLIC 
    "src" 
    "${CMAKE_SOURCE_DIR}/libs/include"
)

#-----------------------------------------------------------------------
# Scenes
#-----------------------------------------------------------------------

set(SCENE_SOURCES)

list(
    APPEND SCENES_SOURCES
    
    "src/scenes/gltf_mesh_viewer/GLTFMeshViewerScene.cpp"
    "src/scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
    
    "src/scenes/demo_3rd_person_scene/Demo3rdPersonScene.hpp"
    "src/scenes/demo_3rd_person_scene/Demo3rdPersonScene.cpp"
    
    # "src/scenes/pbr_scene/PBRScene.cpp"
    # "src/scenes/pbr_scene/PBRScene.hpp"
    
    # "src/scenes/textured_sphere/TexturedSphereScene.cpp"
    # "src/scenes/textured_sphere/TexturedSphereScene.hpp"
    
    # "src/scenes/texture_viewer_scene/TextureViewerScene.hpp"
    # "src/scenes/texture_viewer_scene/TextureViewerScene.cpp"

    # "src/scenes/omnidirectional_shadow_mapping/OmnidirectionalShadowMapping.hpp"
    # "src/scenes/omnidirectional_shadow_mapping/OmnidirectionalShadowMapping.cpp"
)

add_library("Scenes"
    ${SCENES_SOURCES}
)

target_include_directories(
    "Scenes" PUBLIC 
    "src" 
    "${CMAKE_SOURCE_DIR}/libs/include"
)

#-----------------------------------------------------------------------
# Libs
#-----------------------------------------------------------------------

set(LIBS_SOURCES)

list(APPEND LIB_KTX_SOURCES
    LIB_KTX_H
    LIB_KTX_HPP
    LIB_KTX_C
    LIB_KTX_CPP
)

list(
    APPEND LIBS_SOURCES
    #ImGui
    "src/libs/imgui/imconfig.h"
    "src/libs/imgui/imgui_demo.cpp"
    "src/libs/imgui/imgui_draw.cpp"
    "src/libs/imgui/imgui_internal.h"
    "src/libs/imgui/imgui_tables.cpp"
    "src/libs/imgui/imgui_widgets.cpp"
    "src/libs/imgui/imgui.cpp"
    "src/libs/imgui/imgui.h"
    "src/libs/imgui/imstb_rectpack.h"
    "src/libs/imgui/imstb_textedit.h"
    "src/libs/imgui/imstb_truetype.h"
    #StbImage
    "src/libs/stb_image/stb_image_resize.h"
    "src/libs/stb_image/stb_image_write.h"
    "src/libs/stb_image/stb_image.h"
    #TinyGltfLoader
    "src/libs/tiny_gltf_loader/json.hpp"
    "src/libs/tiny_gltf_loader/tiny_gltf_loader.h"
    #TinyObjLoader
    "src/libs/tiny_obj_loader/tiny_obj_loader.h"
    #Ktx
    "src/libs/tiny_ktx/tinyktx.h"
)

if (NOT ANDROID)
list(
    APPEND LIBS_SOURCES
    #SDL
    "src/libs/sdl/SDL.hpp"
)    
endif ()

add_library("Libs"
    ${LIBS_SOURCES}
)

target_include_directories(
    "Libs" PUBLIC 
    "src" 
    "${CMAKE_SOURCE_DIR}/libs/include"
)

#-----------------------------------------------------------------------
# Unit-tests
#-----------------------------------------------------------------------

set(UNIT_TEST_SOURCE)

list(
    APPEND UNIT_TEST_SOURCE
    "unit_tests/catch.hpp"
    "unit_tests/rlutil.h"
    "unit_tests/testMain.cpp"
    "unit_tests/engine/testBedrockMatrix.cpp"
)

#-----------------------------------------------------------------------
# OS specific
#-----------------------------------------------------------------------

if (WINDOWS)

#-----------------------------------------------------------------------
# Windows Executable
#-----------------------------------------------------------------------

    set(WIN_SOURCE)
    
    list(
        APPEND WIN_SOURCE
        "windows/main.cpp"
        "src/Application.hpp"
        "src/Application.cpp"
    )

    file(GLOB VULKAN "${CMAKE_SOURCE_DIR}/libs/win/vulkan-1.lib")

    file(
        GLOB SDL 
        "${CMAKE_SOURCE_DIR}/libs/win/SDL2.lib" 
        "${CMAKE_SOURCE_DIR}/libs/win/SDL2main.lib"
    )

    # file(
    #     GLOB ASAN
    #     "${CMAKE_SOURCE_DIR}/libs/win/clang_rt.asan_dynamic-x86_64.lib"
    #     "${CMAKE_SOURCE_DIR}/libs/win/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib"
    # )

    list(
        APPEND COMPILED_LIBS 
        ${VULKAN}
        ${SDL}
        # ${ASAN}
    )

    file(
        GLOB DLLS 
        "${CMAKE_SOURCE_DIR}/libs/win/SDL2.dll" 
        "${CMAKE_SOURCE_DIR}/libs/win/clang_rt.asan_dynamic-x86_64.dll"
    )

    add_executable(${PROJECT_NAME} ${WIN_SOURCE})

    add_dependencies(${PROJECT_NAME} "Scenes" "Tools" "Engine" "Libs")

    target_link_libraries(${PROJECT_NAME} ${COMPILED_LIBS} "Scenes" "Tools" "Engine" "Libs")

    target_include_directories(
        ${PROJECT_NAME} PUBLIC 
        "windows"
        "src" 
        "${CMAKE_SOURCE_DIR}/libs/include"
    )

    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        ${DLLS}      # <--this is in-file
        ${CMAKE_BINARY_DIR}
    )

#-----------------------------------------------------------------------
# Windows unit tests
#-----------------------------------------------------------------------

    add_executable(
        ${UNIT_TEST_NAME}
        ${UNIT_TEST_SOURCE}
    )

    add_dependencies(${UNIT_TEST_NAME} "Scenes" "Tools" "Engine" "Libs")

    target_link_libraries(${UNIT_TEST_NAME} ${COMPILED_LIBS} "Scenes" "Tools" "Engine" "Libs")

    target_include_directories(
        ${UNIT_TEST_NAME} PUBLIC 
        "unit_tests"
        "src" 
        "${CMAKE_SOURCE_DIR}/libs/include"
    )

    add_custom_command(
        TARGET ${UNIT_TEST_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        ${DLLS}      # <--this is in-file
        ${CMAKE_BINARY_DIR}
    )

elseif(LINUX)

    set(LINUX_SOURCE)
    
    list(
        APPEND LINUX_SOURCE
        "linux/main.cpp"
        "src/Application.hpp"
        "src/Application.cpp"
    )

    add_executable(${PROJECT_NAME} ${LINUX_SOURCE})

    add_dependencies(${PROJECT_NAME} "Scenes" "Tools" "Engine" "Libs")

    target_link_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/libs/linux/sdl/")

    target_link_libraries(${PROJECT_NAME} SDL2 "Scenes" "Tools" "Engine" "Libs")

    target_include_directories(
        ${PROJECT_NAME} PUBLIC 
        "linux"
        "src" 
        "${CMAKE_SOURCE_DIR}/libs/include"
    )

elseif(MACOSX)

#-----------------------------------------------------------------------
# MACOSX Executable
#-----------------------------------------------------------------------
    message(STATUS "Building for macos")

    set(MAC_SOURCE)
    
    list(
        APPEND MAC_SOURCE
        "mac/main.mm"
        "src/Application.hpp"
        "src/Application.cpp"
    )

    add_executable(${PROJECT_NAME} ${MAC_SOURCE})

    add_dependencies(${PROJECT_NAME} "Scenes" "Tools" "Engine" "Libs")

    target_link_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/libs/mac/sdl/")

    target_link_libraries(${PROJECT_NAME} SDL2 "Scenes" "Tools" "Engine" "Libs")

    target_link_libraries(${PROJECT_NAME}
        "-framework CoreGraphics"
        "-framework Foundation"
    )
    
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/libs/mac/libvulkan.1.2.141.dylib")
     
    target_include_directories(
        ${PROJECT_NAME} PUBLIC 
        "mac"
        "src" 
        "${CMAKE_SOURCE_DIR}/libs/include"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_FRAMEWORK_IDENTIFIER co.radx.MFaEngine
    )

    # Copying assets to correct place
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/Debug/MFaEngine.app/Contents/Resources/data/"
        COMMAND cp -r "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/Debug/MFaEngine.app/Contents/Resources/data/"
    )

    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/Release/MFaEngine.app/Contents/Resources/data/"
        COMMAND cp -r "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/Release/MFaEngine.app/Contents/Resources/data/"
    )


elseif(IPHONE)

#-----------------------------------------------------------------------
# IPHONE Executable
#-----------------------------------------------------------------------
    message(STATUS "Building for iphone")

#https://stackoverflow.com/questions/822404/how-to-set-up-cmake-to-build-an-app-for-the-iphone
    set(IPHONE_SOURCE)
        
    list(
        APPEND IPHONE_SOURCE
        "ios/main.mm"
        "ios/ViewController.h"
        "ios/ViewController.mm"
        "ios/AppDelegate.h"
        "ios/AppDelegate.m"
        "src/Application.hpp"
        "src/Application.cpp"
    )

    # set(CMAKE_OSX_SYSROOT iphoneos2.2.1)
    # set(CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD_32_BIT))

    set(MACOSX_BUNDLE_GUI_IDENTIFIER "co.radx.\${PRODUCT_NAME:identifier}")
    
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${IPHONE_SOURCE})

    add_dependencies(${PROJECT_NAME} "Scenes" "Tools" "Engine" "Libs")

    target_link_libraries(${PROJECT_NAME} "Scenes" "Tools" "Engine" "Libs")

    # TODO I think we should use .dylib instead
    target_link_libraries(${PROJECT_NAME}
        "-framework CoreGraphics"
        "-framework UIKit"
        "-framework Metal"
        "-framework QuartzCore"   
        "${CMAKE_SOURCE_DIR}/libs/ios-arm64/libMoltenVK.a"
        "${CMAKE_SOURCE_DIR}/libs/ios-arm64_x86_64-simulator/libMoltenVK.a"
    )

    # target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/libs/ios/libMoltenVK.dylib")

    target_include_directories(
        ${PROJECT_NAME} PUBLIC 
        "ios"
        "src" 
        "${CMAKE_SOURCE_DIR}/libs/include"
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc -ObjC++")
    ELSE()
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc -xobjective-c++")
    ENDIF()

elseif(ANDROID)

#-----------------------------------------------------------------------
# Android library
#-----------------------------------------------------------------------
    message(STATUS "Building for android")    

    set(ANDROID_FILES_ADDRESS "android/app/src/main/jni")

    set(ANDROID_SOURCES)
        
    list(
        APPEND ANDROID_SOURCES
        "${ANDROID_FILES_ADDRESS}/main.cpp"
        "${ANDROID_FILES_ADDRESS}/vulkan_wrapper/vulkan_wrapper.h"
        "${ANDROID_FILES_ADDRESS}/vulkan_wrapper/vulkan_wrapper.cpp"
        "src/Application.hpp"
        "src/Application.cpp"
    )

    include_directories("${ANDROID_FILES_ADDRESS}/vulkan_wrapper")

    # build native_app_glue as a static lib
    set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
    include_directories(${APP_GLUE_DIR})
    add_library(app-glue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c)

    add_library(
        ${PROJECT_NAME} SHARED 
        ${ANDROID_SOURCES} 
        ${ENGINE_SOURCES} 
        ${LIBS_SOURCES}
        ${TOOLS_SOURCES}
        ${SCENES_SOURCES}
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
       -std=c++20 -Wall -Wno-unused-variable \
       -Wno-delete-non-virtual-dtor -DVK_USE_PLATFORM_ANDROID_KHR")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

    function(get_architecture ABI ARCH)
        if (ABI STREQUAL armeabi-v7a)
            set(${ARCH} arm PARENT_SCOPE)
        elseif(ABI STREQUAL arm64-v8a)
            set(${ARCH} aarch64 PARENT_SCOPE)
        elseif(ABI STREQUAL x86)
            set(${ARCH} i686 PARENT_SCOPE)
        elseif(ABI STREQUAL x86_64)
            set(${ARCH} x86_64 PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Unsupported ABI")
        endif()
    endfunction()

    target_link_libraries(${PROJECT_NAME} app-glue log android)
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS -fsanitize=address)

    target_include_directories(
        ${PROJECT_NAME} PUBLIC 
        "src" 
        "${CMAKE_SOURCE_DIR}/libs/include"
    )

    # Asan libs are "somewhere" in the toolchain's root, we try to find the
    # right one for the current architecture and copy it to the libs output dir
    # (so that it will be packed in the apk):

    get_architecture(${ANDROID_ABI} ARCH)
    file(GLOB_RECURSE ASAN_SRC ${ANDROID_TOOLCHAIN_ROOT}/*.asan*${ARCH}*-android.so)
    set(ASAN_DEST ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    add_custom_command(
            TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASAN_SRC} ${ASAN_DEST}
    )

endif()

# Useful syntax:
# file(GLOB_RECURSE LIB_KTX_CPP "src/libs/ktx/*.cpp")
