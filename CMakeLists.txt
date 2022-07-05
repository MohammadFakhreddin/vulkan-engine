cmake_minimum_required (VERSION 3.16.0)

# set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
# set(CMAKE_IOS_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum IOS deployment version")

# set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0")
# set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "11.0")
# For macos and iphone
set(DEPLOYMENT_TARGET "11.0")           # For toolchain target
set(MACOSX_DEPLOYMENT_TARGET "11.0")    # For toolchain target

set(PROJECT_NAME "MFaEngine")

set(UNIT_TEST_NAME "UnitTests")

project ("MFaEngine" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

# OPTION(DEBUG_MODE "Enables debug mode" OFF) # Enabled by default
# if (DEBUG_MODE)
#     add_definitions(-D_DEBUG)
#     set(CMAKE_BUILD_TYPE Debug)
#     message(STATUS "Running cmake for debug build")
# else()
#     add_definitions(-DNDEBUG)
#     set(CMAKE_BUILD_TYPE Release)
#     message(STATUS "Running cmake for release build")
# endif()

set (CMAKE_CXX_STANDARD 20)

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
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
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

#-----------------------------------------------------------------------
# Applications
#-----------------------------------------------------------------------

set(TECHDEMO "TechDemo")
set(PREFAB_EDITOR "PrefabEditor")
set(RAY_TRACING_WEEKEND "RayTracingWeekend")

#-----------------------------------------------------------------------
# GLM
#-----------------------------------------------------------------------

# Vulkan uses 0 to 1 depth instead of -1 to 1 in opengl
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

# Note: Linux On MacOS 10.14 with clang++ compiler, I get executable with SIMD right away.
# On Ubuntu 16.04 with g++ compiler, I will have to use the compiler flag -mavx2 to generate a SIMD executable.
# There are other compiler options for other SIMD instructions here. Without the compiler flag, I still get a serial executable.

# if (UNIX)
#     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
# elseif(WINDOWS)
#     ADD_DEFINITIONS(/arch:AVX2)
# endif ()

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK -DVK_EXAMPLE_XCODE_GENERATED")
endif()

set(glm_DIR "libs/include/glm/cmake/glm")
find_package(glm REQUIRED)

# TODO Use this link to integrate lua into project
# https://gist.github.com/baiyanhuang/4054106

list(
    APPEND COMMON_INCLUDE_DIRECTORIES
    "src"
    "${CMAKE_SOURCE_DIR}/libs/include"
    "${GLM_INCLUDE_DIRS}"
    "libs/include/pxshared/include"
)

#-----------------------------------------------------------------------
# Bedrock
#-----------------------------------------------------------------------

set(BEDROCK_SOURCES)

list(
    APPEND BEDROCK_SOURCES

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
    "src/engine/BedrockSignalTypes.hpp"
    "src/engine/BedrockSignal.hpp"
    "src/engine/BedrockString.hpp"
)

add_library("Bedrock"
    ${BEDROCK_SOURCES}
)

target_include_directories(
    "Bedrock" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
)

#-----------------------------------------------------------------------
# Renderer
#-----------------------------------------------------------------------

set(RENDERER_SOURCES)

list(
    APPEND RENDERER_SOURCES

    # RenderSystem
    "src/engine/render_system/RenderTypes.hpp"
    "src/engine/render_system/RenderTypesFWD.hpp"
    "src/engine/render_system/RenderTypes.cpp"
    "src/engine/render_system/RenderBackend.hpp"
    "src/engine/render_system/RenderBackend.cpp"
    "src/engine/render_system/RenderFrontend.hpp"
    "src/engine/render_system/RenderFrontend.cpp"

    # RenderPass
    "src/engine/render_system/render_passes/RenderPass.hpp"
    "src/engine/render_system/render_passes/RenderPass.cpp"
    "src/engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
    "src/engine/render_system/render_passes/display_render_pass/DisplayRenderPass.cpp"
    "src/engine/render_system/render_passes/point_light_shadow_render_pass/PointLightShadowRenderPass.hpp"
    "src/engine/render_system/render_passes/point_light_shadow_render_pass/PointLightShadowRenderPass.cpp"
    "src/engine/render_system/render_passes/depth_pre_pass/DepthPrePass.hpp"
    "src/engine/render_system/render_passes/depth_pre_pass/DepthPrePass.cpp"
    "src/engine/render_system/render_passes/occlusion_render_pass/OcclusionRenderPass.hpp"
    "src/engine/render_system/render_passes/occlusion_render_pass/OcclusionRenderPass.cpp"
    "src/engine/render_system/render_passes/directional_light_shadow_render_pass/DirectionalLightShadowRenderPass.hpp"
    "src/engine/render_system/render_passes/directional_light_shadow_render_pass/DirectionalLightShadowRenderPass.cpp"

    # RenderResources
    "src/engine/render_system/render_resources/point_light_shadow_resources/PointLightShadowResources.hpp"
    "src/engine/render_system/render_resources/point_light_shadow_resources/PointLightShadowResources.cpp"
    "src/engine/render_system/render_resources/directional_light_shadow_resources/DirectionalLightShadowResources.hpp"
    "src/engine/render_system/render_resources/directional_light_shadow_resources/DirectionalLightShadowResources.cpp"

    # Pipeline
    "src/engine/render_system/pipelines/BasePipeline.hpp"
    "src/engine/render_system/pipelines/BasePipeline.cpp"
    "src/engine/render_system/pipelines/VariantBase.hpp"
    "src/engine/render_system/pipelines/VariantBase.cpp"
    "src/engine/render_system/pipelines/EssenceBase.hpp"
    "src/engine/render_system/pipelines/EssenceBase.cpp"
    "src/engine/render_system/pipelines/DescriptorSetSchema.hpp"
    "src/engine/render_system/pipelines/DescriptorSetSchema.cpp"
    # Debug
    "src/engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
    "src/engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.cpp"
    "src/engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"
    "src/engine/render_system/pipelines/debug_renderer/DebugEssence.cpp"
    "src/engine/render_system/pipelines/debug_renderer/DebugVariant.hpp"
    "src/engine/render_system/pipelines/debug_renderer/DebugVariant.cpp"
    # PBR
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.cpp"
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Essence.hpp"
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Essence.cpp"
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
    "src/engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.cpp"
    # Particle
    "src/engine/render_system/pipelines/particle/ParticlePipeline.hpp"
    "src/engine/render_system/pipelines/particle/ParticlePipeline.cpp"
    "src/engine/render_system/pipelines/particle/ParticleEssence.hpp"
    "src/engine/render_system/pipelines/particle/ParticleEssence.cpp"
    "src/engine/render_system/pipelines/particle/ParticleVariant.hpp"
    "src/engine/render_system/pipelines/particle/ParticleVariant.cpp"

    "src/engine/render_system/pipelines/particle/FireEssence.hpp"
    "src/engine/render_system/pipelines/particle/FireEssence.cpp"
)

add_library("Renderer" ${RENDERER_SOURCES})

target_include_directories(
    "Renderer" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
)

#-----------------------------------------------------------------------
# Entity system
#-----------------------------------------------------------------------

set(ENTITY_SYSTEM_SOURCES)

list(
    APPEND ENTITY_SYSTEM_SOURCES
    # Entity system
    "src/engine/entity_system/EntitySystemTypes.hpp"
    "src/engine/entity_system/EntitySystem.hpp"
    "src/engine/entity_system/EntitySystem.cpp"
    "src/engine/entity_system/EntityHandle.hpp"
    "src/engine/entity_system/Entity.hpp"
    "src/engine/entity_system/Entity.cpp"
    "src/engine/entity_system/Component.hpp"
    "src/engine/entity_system/Component.cpp"

    # Components    // TODO: Move this to a separate location
    "src/engine/entity_system/components/TransformComponent.hpp"
    "src/engine/entity_system/components/TransformComponent.cpp"
    
    "src/engine/entity_system/components/ColliderComponent.hpp"
    "src/engine/entity_system/components/ColliderComponent.cpp"
    "src/engine/entity_system/components/BoxColliderComponent.hpp"
    "src/engine/entity_system/components/BoxColliderComponent.cpp"
    "src/engine/entity_system/components/RigidbodyComponent.hpp"
    "src/engine/entity_system/components/RigidbodyComponent.cpp"

    "src/engine/entity_system/components/RendererComponent.hpp"
    "src/engine/entity_system/components/RendererComponent.cpp"
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
    "src/engine/entity_system/components/ColorComponent.cpp"

    "src/engine/entity_system/components/PointLightComponent.hpp"
    "src/engine/entity_system/components/PointLightComponent.cpp"
    "src/engine/entity_system/components/DirectionalLightComponent.hpp"
    "src/engine/entity_system/components/DirectionalLightComponent.cpp"
)


add_library("EntitySystem"
    ${ENTITY_SYSTEM_SOURCES}
)

target_include_directories(
    "EntitySystem" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
)

#-----------------------------------------------------------------------
# AssetSystem
#-----------------------------------------------------------------------

set(ASSET_SYSTEM_SOURCES)

list(
    APPEND ASSET_SYSTEM_SOURCES
    "src/engine/asset_system/AssetTypes.hpp"
    "src/engine/asset_system/AssetBase.hpp"
    "src/engine/asset_system/AssetBase.cpp"
    "src/engine/asset_system/AssetBaseMesh.hpp"
    "src/engine/asset_system/AssetBaseMesh.cpp"
    "src/engine/asset_system/Asset_PBR_Mesh.hpp"
    "src/engine/asset_system/Asset_PBR_Mesh.cpp"
    "src/engine/asset_system/AssetDebugMesh.hpp"
    "src/engine/asset_system/AssetDebugMesh.cpp"
    "src/engine/asset_system/AssetParticleMesh.hpp"
    "src/engine/asset_system/AssetParticleMesh.cpp"
    "src/engine/asset_system/AssetShader.hpp"
    "src/engine/asset_system/AssetShader.cpp"
    "src/engine/asset_system/AssetTexture.hpp"
    "src/engine/asset_system/AssetTexture.cpp"
    "src/engine/asset_system/AssetModel.hpp"
    "src/engine/asset_system/AssetModel.cpp"
)

add_library("AssetSystem" ${ASSET_SYSTEM_SOURCES})

target_include_directories(
    "AssetSystem" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
)

#-----------------------------------------------------------------------
# Engine
#-----------------------------------------------------------------------

set(ENGINE_SOURCES)

list(
    APPEND ENGINE_SOURCES

    # Job system
    "src/engine/job_system/ThreadPool.hpp"
    "src/engine/job_system/ThreadPool.cpp"
    "src/engine/job_system/ThreadSafeQueue.hpp"
    "src/engine/job_system/JobSystem.hpp"
    "src/engine/job_system/JobSystem.cpp"
    "src/engine/job_system/JobSystemTypes.hpp"
    "src/engine/job_system/ScopeLock.hpp"
    "src/engine/job_system/ScopeLock.cpp"
    "src/engine/job_system/TaskTracker.hpp"
    "src/engine/job_system/TaskTracker.cpp"

    # UI System
    "src/engine/ui_system/UI_System.hpp"
    "src/engine/ui_system/UI_System.cpp"
    "src/engine/ui_system/UI_Pipeline.hpp"
    "src/engine/ui_system/UI_Pipeline.cpp"

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

    # Application
    "src/Application.hpp"
    "src/Application.cpp"

    # ResourceManager
    "src/engine/resource_manager/ResourceManager.hpp"
    "src/engine/resource_manager/ResourceManager.cpp"
)

add_library("Engine"
    ${ENGINE_SOURCES}
)

target_include_directories(
    "Engine" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
)

#-----------------------------------------------------------------------
# JobSystem
#-----------------------------------------------------------------------

set(JOB_SYSTEM_SOURCES)

list(
    APPEND JOB_SYSTEM_SOURCES
    "src/engine/job_system/ThreadPool.hpp"
    "src/engine/job_system/ThreadPool.cpp"
    "src/engine/job_system/ThreadSafeQueue.hpp"
    "src/engine/job_system/JobSystem.hpp"
    "src/engine/job_system/JobSystem.cpp"
    "src/engine/job_system/JobSystemTypes.hpp"
    "src/engine/job_system/ScopeLock.hpp"
    "src/engine/job_system/ScopeLock.cpp"
    "src/engine/job_system/TaskTracker.hpp"
    "src/engine/job_system/TaskTracker.cpp"
)

add_library("JobSystem"
    ${JOB_SYSTEM_SOURCES}
)

target_include_directories(
    "JobSystem" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
)

#-----------------------------------------------------------------------
# Physics
#-----------------------------------------------------------------------

set(PHYSICS_SOURCES)

list(
    APPEND PHYSICS_SOURCES
    "src/engine/physics/PhysicsTypes.hpp"
    "src/engine/physics/PhysicsTypes.cpp"
    "src/engine/physics/Physics.hpp"
    "src/engine/physics/Physics.cpp"
    "src/engine/physics/LayerMask.hpp"
    "src/engine/physics/LayerMask.cpp"
    "src/engine/physics/LayerMaskDB.hpp"
    "src/engine/physics/LayerMaskDB.cpp"
)

add_library("Physics"
    ${PHYSICS_SOURCES}
)

target_include_directories(
    "Physics" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
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
    "src/tools/Prefab.cpp"          # Maybe we should move prefab to engine
    "src/tools/Prefab.hpp"
    "src/tools/PrefabFileStorage.cpp"
    "src/tools/PrefabFileStorage.hpp"
    "src/tools/JsonUtils.hpp"
    "src/tools/JsonUtils.cpp"
)

add_library("Tools"
    ${TOOLS_SOURCES}
)

target_include_directories(
    "Tools" PUBLIC
    ${COMMON_INCLUDE_DIRECTORIES}
)

#-----------------------------------------------------------------------
# Tech-demo sources
#-----------------------------------------------------------------------

set(TECHDEMO_SOURCES)

list(
    APPEND TECHDEMO_SOURCES

    "applications/techdemo/TechDemoApplication.hpp"
    "applications/techdemo/TechDemoApplication.cpp"

    "applications/techdemo/scenes/gltf_mesh_viewer/GLTFMeshViewerScene.cpp"
    "applications/techdemo/scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"

    "applications/techdemo/scenes/demo_3rd_person_scene/Demo3rdPersonScene.hpp"
    "applications/techdemo/scenes/demo_3rd_person_scene/Demo3rdPersonScene.cpp"

    "applications/techdemo/scenes/particle_fire_scene/ParticleFireScene.hpp"
    "applications/techdemo/scenes/particle_fire_scene/ParticleFireScene.cpp"

    # "src/scenes/pbr_scene/PBRScene.cpp"
    # "src/scenes/pbr_scene/PBRScene.hpp"

    # "src/scenes/textured_sphere/TexturedSphereScene.cpp"
    # "src/scenes/textured_sphere/TexturedSphereScene.hpp"

    # "src/scenes/texture_viewer_scene/TextureViewerScene.hpp"
    # "src/scenes/texture_viewer_scene/TextureViewerScene.cpp"

    # "src/scenes/omnidirectional_shadow_mapping/OmnidirectionalShadowMapping.hpp"
    # "src/scenes/omnidirectional_shadow_mapping/OmnidirectionalShadowMapping.cpp"
)

#-----------------------------------------------------------------------
# Ray-tracing sources
#-----------------------------------------------------------------------

set(RAY_TRACING_SOURCES)

list(APPEND RAY_TRACING_SOURCES
    "applications/ray_tracing_weekend/RayTracingWeekendApplication.hpp"
    "applications/ray_tracing_weekend/RayTracingWeekendApplication.cpp"
    "applications/ray_tracing_weekend/ray/Ray.hpp"
    "applications/ray_tracing_weekend/ray/Ray.cpp"
)

#-----------------------------------------------------------------------
# Prefab editor sources
#-----------------------------------------------------------------------

set(PREFAB_EDITOR_SOURCES)

list(APPEND PREFAB_EDITOR_SOURCES
    "applications/prefab_editor/PrefabEditorApplication.hpp"
    "applications/prefab_editor/PrefabEditorApplication.cpp"
    "applications/prefab_editor/scenes/PrefabEditorScene.hpp"
    "applications/prefab_editor/scenes/PrefabEditorScene.cpp"
    "applications/prefab_editor/WindowsApi.hpp"
    "applications/prefab_editor/WindowsApi.cpp"
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
    "src/libs/imgui/imgui_stdlib.cpp"
    "src/libs/imgui/imgui_stdlib.h"
    "src/libs/imgui/imstb_rectpack.h"
    "src/libs/imgui/imstb_textedit.h"
    "src/libs/imgui/imstb_truetype.h"
    #StbImage
    "src/libs/stb_image/stb_image_resize.h"
    "src/libs/stb_image/stb_image_write.h"
    "src/libs/stb_image/stb_image.h"
    #TinyGltfLoader
    "src/libs/tiny_gltf_loader/tiny_gltf_loader.h"
    #TinyObjLoader
    "src/libs/tiny_obj_loader/tiny_obj_loader.h"
    #Ktx
    "src/libs/tiny_ktx/tinyktx.h"
    #nlohmann
    "src/libs/nlohmann/json_fwd.hpp"
    "src/libs/nlohmann/json.hpp"
    #vma
    "src/libs/vma/vk_mem_alloc.h"
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
    ${COMMON_INCLUDE_DIRECTORIES}
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
    "unit_tests/engine/testSIMD.cpp"
    "unit_tests/engine/testPath.cpp"
)

#-----------------------------------------------------------------------
# OS specific
#-----------------------------------------------------------------------


if (WINDOWS)

#-----------------------------------------------------------------------
# Windows Executable
#-----------------------------------------------------------------------

    set(link_to_target_sources)
    set(link_to_target_directories)
    set(link_to_target_sources)
    function(link_to_target executable_name)
        message(STATUS "===========================================")    
        message(STATUS "Linking dependancies to ${executable_name} ")
        message(STATUS "===========================================")

        add_executable(${executable_name} ${link_to_target_sources})
        
        add_dependencies(${executable_name} "Tools" "Bedrock" "Renderer" "AssetSystem" "EntitySystem" "Engine" "Libs" "JobSystem" "Physics")
        
        target_link_libraries(${executable_name} ${COMPILED_LIBS} "Tools" "Bedrock" "Renderer" "AssetSystem" "EntitySystem" "Engine" "Libs" "JobSystem" "Physics")
        
        target_include_directories(${executable_name} PUBLIC ${COMMON_INCLUDE_DIRECTORIES} ${link_to_target_directories})
        
        add_custom_command(
            TARGET ${executable_name} POST_BUILD                # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different       # which executes "cmake - E copy_if_different..."
            ${DLLS_RELEASE}                                     # <--this is in-file
            "${CMAKE_BINARY_DIR}/Release"
        )

        add_custom_command(
            TARGET ${executable_name} POST_BUILD                # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different       # which executes "cmake - E copy_if_different..."
            ${DLLS_DEBUG}                                       # <--this is in-file
            "${CMAKE_BINARY_DIR}/Debug"
        )

        foreach (_lib ${PHYSX_LIBS})
            message(STATUS "Linking debug library at ${PHYSX_LIBS_DEBUG_PATH}/${_lib}")
            target_link_libraries(${executable_name} debug "${PHYSX_LIBS_DEBUG_PATH}/${_lib}")
        endforeach()

        foreach (_lib ${PHYSX_LIBS})
            message(STATUS "Linking release library at ${PHYSX_LIBS_RELEASE_PATH}/${_lib}")
            target_link_libraries(${executable_name} optimized "${PHYSX_LIBS_RELEASE_PATH}/${_lib}")
        endforeach()

        message(STATUS "===========================================")

    endfunction()

    message(STATUS "Building for windows")

    set(WIN_SOURCES)
    set(WIN_INCLUDE_DIRECTORIES)

    list(APPEND WIN_SOURCES
        "windows/main.cpp"
    )

    list(APPEND WIN_INCLUDE_DIRECTORIES
        "windows"
    )

    file(GLOB VULKAN "${CMAKE_SOURCE_DIR}/libs/win/vulkan-1.lib")

    file(
        GLOB SDL
        "${CMAKE_SOURCE_DIR}/libs/win/SDL2.lib"
        "${CMAKE_SOURCE_DIR}/libs/win/SDL2main.lib"
    )

    set (PHYSX_LIBS_DEBUG_PATH "${CMAKE_SOURCE_DIR}/libs/win/physx/debug-libs")

    set (PHYSX_LIBS_RELEASE_PATH "${CMAKE_SOURCE_DIR}/libs/win/physx/release-libs")

    set(PHYSX_LIBS
        "PhysX_64.lib"
        "PhysXCommon_64.lib"
        "PhysXFoundation_64.lib"
        "PhysXExtensions_static_64.lib"
        "PhysXCooking_64.lib"
        "PhysXPvdSDK_static_64.lib"
        "PhysXTask_static_64.lib"
        "PhysXVehicle_static_64.lib"
        "PhysXCharacterKinematic_static_64.lib"
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

    file (GLOB DLLS_COMMON
        "${CMAKE_SOURCE_DIR}/libs/win/SDL2.dll"
        # "${CMAKE_SOURCE_DIR}/libs/win/clang_rt.asan_dynamic-x86_64.dll"
    )

    file(GLOB DLLS_DEBUG
        ${DLLS_COMMON}
        "${PHYSX_LIBS_DEBUG_PATH}/PhysX_64.dll"
        "${PHYSX_LIBS_DEBUG_PATH}/PhysXFoundation_64.dll"
        "${PHYSX_LIBS_DEBUG_PATH}/PhysXCommon_64.dll"
    )

    file(GLOB DLLS_RELEASE
        ${DLLS_COMMON}
        "${PHYSX_LIBS_RELEASE_PATH}/PhysX_64.dll"
        "${PHYSX_LIBS_RELEASE_PATH}/PhysXFoundation_64.dll"
        "${PHYSX_LIBS_RELEASE_PATH}/PhysXCommon_64.dll"
    )

    # Techdemo executable

    unset(link_to_target_sources)
    list(APPEND link_to_target_sources
        ${WIN_SOURCES}
        ${TECHDEMO_SOURCES}
    )
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        ${WIN_INCLUDE_DIRECTORIES}
        "applications/techdemo"
    )
    link_to_target(${TECHDEMO})
    target_compile_definitions(${TECHDEMO} PRIVATE TECHDEMO)

    # Prefab editor executable

    unset(link_to_target_sources)
    list(APPEND link_to_target_sources ${WIN_SOURCES} ${PREFAB_EDITOR_SOURCES})
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        ${WIN_INCLUDE_DIRECTORIES}
        "applications/prefab_editor"
    )
    link_to_target(${PREFAB_EDITOR})
    target_compile_definitions(${PREFAB_EDITOR} PRIVATE PREFAB_EDITOR)

    # Ray tracing executable
    
    unset(link_to_target_sources)
    list(APPEND link_to_target_sources 
        ${WIN_SOURCES} 
        ${RAY_TRACING_SOURCES}
    )
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories 
        ${WIN_INCLUDE_DIRECTORIES}
        "applications/ray_tracing_weekend"
    )
    link_to_target(${RAY_TRACING_WEEKEND})
    target_compile_definitions(${RAY_TRACING_WEEKEND} PRIVATE RAY_TRACING_WEEKEND)

#-----------------------------------------------------------------------
# Windows unit tests
#-----------------------------------------------------------------------

    unset(link_to_target_sources)
    list(APPEND link_to_target_sources ${UNIT_TEST_SOURCE})
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        "unit_tests"
    )
    link_to_target(${UNIT_TEST_NAME})
    target_compile_definitions(${UNIT_TEST_NAME} PRIVATE UNIT_TEST PUBLIC ENABLE_SIMD)

elseif(LINUX)

#-----------------------------------------------------------------------
# LINUX Executable
#-----------------------------------------------------------------------

# TODO Use link to target function
    message(STATUS "Building for linux")

    set(link_to_target_directories)
    set(link_to_target_sources)
    function(link_to_target executable_name)
        message(STATUS "Linking dependancies to ${executable_name} ")
        add_executable(${executable_name} ${link_to_target_sources})
        add_dependencies(${executable_name}  "Renderer" "EntitySystem" "Engine" "Tools" "AssetSystem" "Bedrock" "Libs" "JobSystem" "Physics")
        target_link_libraries(${executable_name} "Renderer" "EntitySystem" "Engine" "Tools" "AssetSystem" "Bedrock" "Libs" "JobSystem" "Physics")
        target_link_libraries(${executable_name} "${CMAKE_SOURCE_DIR}/libs/linux/sdl/libSDL2.a")
        target_link_libraries(${executable_name} "${CMAKE_SOURCE_DIR}/libs/linux/sdl/libSDL2main.a")
        target_link_libraries(${executable_name} "${CMAKE_SOURCE_DIR}/libs/linux/libvulkan.so.1.3.204")
        target_include_directories(
            ${executable_name} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )
        target_link_libraries(${executable_name} ${CMAKE_DL_LIBS})
        # target_link_directories(${executable_name} PUBLIC "${CMAKE_SOURCE_DIR}/libs/linux/sdl/")
        # target_include_directories(${executable_name} PUBLIC "${CMAKE_SOURCE_DIR}/libs/linux/sdl/")
        target_include_directories(${executable_name} PUBLIC ${COMMON_INCLUDE_DIRECTORIES} ${link_to_target_directories})

        target_link_libraries(${executable_name} X11)

    endfunction()

    message(STATUS "Building for linux")

    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIRS})

    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)

    set(LINUX_SOURCE)
    set(LINUX_INCLUDE_DIRECTORIES)

    list(
        APPEND LINUX_SOURCE
        "linux/main.cpp"
    )

    list(
        APPEND LINUX_INCLUDE_DIRECTORIES
        "linux"
    )

    # Techdemo
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        ${LINUX_INCLUDE_DIRECTORIES}
        "applications/techdemo"
    )
    unset(link_to_target_sources)
    list(APPEND link_to_target_sources
        ${LINUX_SOURCE}
        ${TECHDEMO_SOURCES}
    )
    link_to_target(${TECHDEMO})
    target_compile_definitions(${TECHDEMO} PRIVATE TECHDEMO)

    # Editor
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        ${LINUX_INCLUDE_DIRECTORIES}
        "applications/prefab_editor"
    )
    unset(link_to_target_sources)
    list(APPEND link_to_target_sources
        ${LINUX_SOURCE}
        ${PREFAB_EDITOR_SOURCES}
    )
    link_to_target(${PREFAB_EDITOR})
    target_compile_definitions(${PREFAB_EDITOR} PRIVATE PREFAB_EDITOR)

elseif(MACOSX)

#-----------------------------------------------------------------------
# MACOSX Executable
#-----------------------------------------------------------------------

    set(link_to_target_directories)
    set(link_to_target_sources)
    function(link_to_target executable_name)
        message(STATUS "Linking dependancies to ${executable_name} ")
        add_executable(${executable_name} ${link_to_target_sources})
        add_dependencies(${executable_name} "Tools" "Bedrock" "Renderer" "EntitySystem" "Engine" "AssetSystem" "Libs" "JobSystem" "Physics")
        target_link_libraries(${executable_name} "${CMAKE_SOURCE_DIR}/libs/mac/libvulkan.1.3.211.dylib")
        target_link_libraries(${executable_name} "${CMAKE_SOURCE_DIR}/libs/mac/libMoltenVK.dylib")
        target_link_libraries(${executable_name}
            "-framework CoreGraphics"
            "-framework Foundation"
        )
        target_link_libraries(${executable_name} SDL2 "Tools" "Bedrock" "Renderer" "EntitySystem" "Engine" "AssetSystem" "Libs" "JobSystem" "Physics")
        target_link_directories(${executable_name} PUBLIC "${CMAKE_SOURCE_DIR}/libs/mac/sdl/")
        target_include_directories(${executable_name} PUBLIC "${CMAKE_SOURCE_DIR}/libs/mac/sdl/")
        target_include_directories(${executable_name} PUBLIC ${COMMON_INCLUDE_DIRECTORIES} ${link_to_target_directories})
        set_target_properties(${executable_name} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_FRAMEWORK_IDENTIFIER "co.radx.${executable_name}"
        )

        # TODO: Clean this part
        if(CMAKE_GENERATOR STREQUAL Xcode)
            # Copying assets to correct place
            add_custom_command(
                TARGET ${executable_name} POST_BUILD        # Adds a post-build event to MyTest
                COMMAND mkdir -p "${CMAKE_BINARY_DIR}/Debug/${executable_name}.app/Contents/Resources/data/"
                COMMAND cp -r "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/Debug/${executable_name}.app/Contents/Resources/data/"
            )

            add_custom_command(
                TARGET ${executable_name} POST_BUILD        # Adds a post-build event to MyTest
                COMMAND mkdir -p "${CMAKE_BINARY_DIR}/Release/${executable_name}.app/Contents/Resources/data/"
                COMMAND cp -r "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/Release/${executable_name}.app/Contents/Resources/data/"
            )
        else()
            add_custom_command(
                TARGET ${executable_name} POST_BUILD        # Adds a post-build event to MyTest
                COMMAND mkdir -p "${CMAKE_BINARY_DIR}/${executable_name}.app/Contents/Resources/data/"
                COMMAND cp -r "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/${executable_name}.app/Contents/Resources/data/"
            )

            add_custom_command(
                TARGET ${executable_name} POST_BUILD        # Adds a post-build event to MyTest
                COMMAND mkdir -p "${CMAKE_BINARY_DIR}/${executable_name}.app/Contents/Resources/data/"
                COMMAND cp -r "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/${executable_name}.app/Contents/Resources/data/"
            )
        endif()

        if(CMAKE_GENERATOR STREQUAL Xcode)
            target_compile_definitions(${executable_name} PRIVATE XCODE)
        endif()

    endfunction()

    message(STATUS "Building for macos")

    set(MAC_SOURCE)
    set(MAC_INCLUDE_DIRECTORIES)

    list(
        APPEND MAC_SOURCE
        "mac/main.mm"
        "mac/BedrockPath.mm"
    )

    list(APPEND MAC_INCLUDE_DIRECTORIES
        "mac"
    )

    # Techdemo
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        ${MAC_INCLUDE_DIRECTORIES}
        "applications/techdemo"
    )
    unset(link_to_target_sources)
    list(APPEND link_to_target_sources
        ${MAC_SOURCE}
        ${TECHDEMO_SOURCES}
    )
    link_to_target(${TECHDEMO})
    target_compile_definitions(${TECHDEMO} PRIVATE TECHDEMO)

    # RayTracingInOneWeekedn
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories 
        ${MAC_INCLUDE_DIRECTORIES}
        "applications/ray_tracing_weekend"
    )
    unset(link_to_target_sources)
    list(APPEND link_to_target_sources 
        ${MAC_SOURCE} 
        ${RAY_TRACING_SOURCES}
    )
    link_to_target(${RAY_TRACING_WEEKEND})
    target_compile_definitions(${RAY_TRACING_WEEKEND} PRIVATE RAY_TRACING_WEEKEND)

    # Editor
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        ${MAC_INCLUDE_DIRECTORIES}
        "applications/prefab_editor"
    )
    unset(link_to_target_sources)
    list(APPEND link_to_target_sources
        ${MAC_SOURCE}
        ${PREFAB_EDITOR_SOURCES}
    )
    link_to_target(${PREFAB_EDITOR})
    target_compile_definitions(${PREFAB_EDITOR} PRIVATE TECHDEMO)

    # Unit-tests
    unset(link_to_target_sources)
    list(APPEND link_to_target_sources ${UNIT_TEST_SOURCE} "mac/BedrockPath.mm")
    unset(link_to_target_directories)
    list(APPEND link_to_target_directories
        "unit_tests"
    )
    link_to_target(${UNIT_TEST_NAME})
    target_compile_definitions(${UNIT_TEST_NAME} PRIVATE UNIT_TEST)

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

    add_executable(${TECHDEMO} MACOSX_BUNDLE ${IPHONE_SOURCE})

    add_dependencies(${TECHDEMO} "Tools" "Bedrock" "Renderer" "EntitySystem" "Engine" "AssetSystem" "Libs" "JobSystem" "Physics")

    target_link_libraries(${TECHDEMO} "Tools" "Bedrock" "Renderer" "EntitySystem" "Engine" "AssetSystem" "Libs" "JobSystem" "Physics")

    # TODO I think we should use .dylib instead
    target_link_libraries(${TECHDEMO}
        "-framework CoreGraphics"
        "-framework UIKit"
        "-framework Metal"
        "-framework QuartzCore"
        "${CMAKE_SOURCE_DIR}/libs/ios-arm64/libMoltenVK.a"
        "${CMAKE_SOURCE_DIR}/libs/ios-arm64_x86_64-simulator/libMoltenVK.a"
    )

    # target_link_libraries(${TECHDEMO} "${CMAKE_SOURCE_DIR}/libs/ios/libMoltenVK.dylib")

    target_include_directories(
        ${TECHDEMO} PUBLIC
        "ios"
        "${COMMON_INCLUDE_DIRECTORIES}"
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
        ${TECHDEMO} SHARED
        ${ANDROID_SOURCES}
        ${ENGINE_SOURCES}
        ${JOB_SYSTEM_SOURCES}
        ${PHYSICS_SOURCES}
        ${LIBS_SOURCES}
        ${TOOLS_SOURCES}
        ${SCENES_SOURCES}
        ${ASSET_SYSTEM_SOURCES}
        ${ENTITY_SYSTEM_SOURCES}
        ${RENDERER_SOURCES}
        ${BEDROCK_SOURCES}
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
       -Wall -Wno-unused-variable \
       -Wno-delete-non-virtual-dtor  \
       -DVK_USE_PLATFORM_ANDROID_KHR"
    )
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

    target_link_libraries(${TECHDEMO} app-glue log android)
    # target_compile_options(${TECHDEMO} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
    # set_target_properties(${TECHDEMO} PROPERTIES LINK_FLAGS -fsanitize=address)

    target_include_directories(
        ${TECHDEMO} PUBLIC
        "${COMMON_INCLUDE_DIRECTORIES}"
    )

    # Asan libs are "somewhere" in the toolchain's root, we try to find the
    # right one for the current architecture and copy it to the libs output dir
    # (so that it will be packed in the apk):

    # get_architecture(${ANDROID_ABI} ARCH)
    # file(GLOB_RECURSE ASAN_SRC ${ANDROID_TOOLCHAIN_ROOT}/*.asan*${ARCH}*-android.so)
    # set(ASAN_DEST ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    # add_custom_command(
    #         TARGET ${TECHDEMO} PRE_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASAN_SRC} ${ASAN_DEST}
    # )

endif()

# Useful syntax:
# file(GLOB_RECURSE LIB_KTX_CPP "src/libs/ktx/*.cpp")
