cmake_minimum_required (VERSION 3.10)

set(${PROJECT_NAME} "MFaEngine")

set(${UNIT_TEST_NAME} "UnitTests")

project ("MFaEngine" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)
# TODO We need 2 solutions, 1 for Engine and 1 for test cases
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
# set (CMAKE_CONFIGURATION_TYPES Debug;Release CACHE INTERNAL "" FORCE)
# set (CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE INTERNAL "" FORCE)

if(APPLE)
   if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(MACOSX TRUE)
   elseif()
      set(IPHONE TRUE)
   endif()
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS TRUE)
endif()

if(WINDOWS)
  add_definitions (/WX)           # Warnings as errors
endif()

#-----------------------------------------------------------------------
# Engine
#-----------------------------------------------------------------------

set(ENGINE_SOURCES)

# file(GLOB_RECURSE ENGINE_HEADER_H "src/engine/*.h")
# file(GLOB_RECURSE ENGINE_HEADER_HPP "src/engine/*.hpp")
# file(GLOB_RECURSE ENGINE_CPP "src/engine/*.cpp")

list(
    APPEND ENGINE_SOURCES
    "src/engine/BedrockAssert.cpp"
    "src/engine/BedrockAssert.hpp"
    "src/engine/BedrockCommon.hpp"
    "src/engine/BedrockFileSystem.hpp"
    "src/engine/BedrockFileSystem.cpp"
    "src/engine/BedrockLog.hpp"
    "src/engine/BedrockMath.hpp"
    "src/engine/BedrockMatrix.hpp"
    "src/engine/BedrockMemory.hpp"
    "src/engine/BedrockMemory.cpp"
    "src/engine/BedrockPlatforms.hpp"
    "src/engine/BedrockPlatforms.cpp"
    "src/engine/DrawableObject.hpp"
    "src/engine/DrawableObject.cpp"
    "src/engine/FoundationAsset.hpp"
    "src/engine/FoundationAsset.cpp"
    "src/engine/RenderBackend.hpp"
    "src/engine/RenderBackend.cpp"
    "src/engine/RenderFrontend.hpp"
    "src/engine/RenderFrontend.cpp"
    "src/engine/Scene.hpp"
    "src/engine/Scene.cpp"
    "src/engine/UISystem.hpp"
    "src/engine/UISystem.cpp"
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
    "src/scenes/pbr_scene/PBRScene.cpp"
    "src/scenes/pbr_scene/PBRScene.hpp"
    "src/scenes/textured_sphere/TexturedSphereScene.cpp"
    "src/scenes/textured_sphere/TexturedSphereScene.hpp"
    "src/scenes/SpecularHighlightScene.hpp"
    "src/scenes/TextureViewerScene.hpp"
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
)

add_library("Libs"
    ${LIBS_SOURCES}
)

target_include_directories(
    "Libs" PUBLIC 
    "src" 
    "${CMAKE_SOURCE_DIR}/libs/include"
)

#-----------------------------------------------------------------------
# OS specific
#-----------------------------------------------------------------------

if (WINDOWS)

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

    file(
        GLOB IMGUI
        "${CMAKE_SOURCE_DIR}/libs/win/imgui.lib"
    )

    list(
        APPEND COMPILED_LIBS 
        ${VULKAN}
        ${SDL}
    )

    file(
        GLOB DLLS 
        "${CMAKE_SOURCE_DIR}/libs/win/SDL2.dll" 
    )

    add_executable(${PROJECT_NAME} ${WIN_HEADER} ${WIN_SOURCE})

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

    # add_executable(
    #     ${UNIT_TEST_NAME}
    #     ${UNITTEST_SOURCE}
    #     ${UNITTEST_HEADER}
    #     ${APP_SOURCE}
    #     ${APP_HEADER}
    # )

    # target_link_libraries(${UNIT_TEST_NAME} ${COMPILED_LIBS})

    # add_custom_command(
    #     TARGET ${UNIT_TEST_NAME} POST_BUILD        # Adds a post-build event to MyTest
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
    #     ${DLLS}      # <--this is in-file
    #     ${CMAKE_BINARY_DIR}
    # )

endif()
# set(APP_SOURCE)
# set(APP_HEADER)

# set(UNITTEST_SOURCE)
# set(UNITTEST_HEADER)

# # These files get included for build process
# file(GLOB_RECURSE SRC_HEADER_H "src/*.h")
# file(GLOB_RECURSE SRC_HEADER_HPP "src/*.hpp")
# file(GLOB_RECURSE SRC_CPP "src/*.cpp")

# file(GLOB_RECURSE UNITTEST_HEADER_H "unit_tests/*.h")
# file(GLOB_RECURSE UNITTEST_HEADER_HPP "unit_tests/*.hpp")
# file(GLOB_RECURSE UNITTEST_CPP "unit_tests/*.cpp")

# list(
#     APPEND APP_SOURCE
#     ${SRC_CPP}
# )

# list(
#     APPEND APP_HEADER
#     ${SRC_HEADER_H}
#     ${SRC_HEADER_HPP}
# )

# list(
#     APPEND UNITTEST_SOURCE
#     ${UNITTEST_CPP}
# )

# list(
#     APPEND UNITTEST_HEADER
#     ${UNITTEST_HEADER_H}
#     ${UNITTEST_HEADER_HPP}
# )

# # Includes these 2 directory for importing libraries like VULAKN and SDL that get installed without include path
# # include_directories ("src" "${CMAKE_SOURCE_DIR}/libs/include")
# link_directories(${LIBRARY_OUTPUT_PATH} ${LIB_ROOT})

# if(WINDOWS)

#     set(WIN_SOURCE)
#     set(WIN_HEADER)

#     file(GLOB_RECURSE WIN_HEADER_H "windows/*.h")
#     file(GLOB_RECURSE WIN_HEADER_HPP "windows/*.hpp")
#     file(GLOB_RECURSE WIN_CPP "windows/*.cpp")

#     list(
#         APPEND WIN_HEADER
#         ${WIN_HEADER_H}
#         ${WIN_HEADER_HPP}
#     )
    
#     list(
#         APPEND WIN_SOURCE
#         ${WIN_SOURCE}
#     )

#     file(GLOB VULKAN "${CMAKE_SOURCE_DIR}/libs/win/vulkan-1.lib")
#     file(
#         GLOB SDL 
#         "${CMAKE_SOURCE_DIR}/libs/win/SDL2.lib" 
#         "${CMAKE_SOURCE_DIR}/libs/win/SDL2main.lib"
#     )
#     file(
#         GLOB IMGUI
#         "${CMAKE_SOURCE_DIR}/libs/win/imgui.lib"
#     )
#     list(
#         APPEND COMPILED_LIBS 
#         ${VULKAN}
#         ${SDL}
#     )

#     file(
#         GLOB DLLS 
#         "${CMAKE_SOURCE_DIR}/libs/win/SDL2.dll" 
#     )

#     add_executable(${PROJECT_NAME} ${APP_HEADER} ${APP_SOURCE} ${WIN_HEADER} ${WIN_SOURCE})

#     target_link_libraries(${PROJECT_NAME} ${COMPILED_LIBS})

#     add_custom_command(
#         TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
#         ${DLLS}      # <--this is in-file
#         ${CMAKE_BINARY_DIR}
#     )

#     # add_executable(
#     #     ${UNIT_TEST_NAME}
#     #     ${UNITTEST_SOURCE}
#     #     ${UNITTEST_HEADER}
#     #     ${APP_SOURCE}
#     #     ${APP_HEADER}
#     # )

#     # target_link_libraries(${UNIT_TEST_NAME} ${COMPILED_LIBS})

#     # add_custom_command(
#     #     TARGET ${UNIT_TEST_NAME} POST_BUILD        # Adds a post-build event to MyTest
#     #     COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
#     #     ${DLLS}      # <--this is in-file
#     #     ${CMAKE_BINARY_DIR}
#     # )
# elseif(MACOSX)

#   # TODO Libs

#   find_package(SDL2 REQUIRED)
#   find_package(VULAKN REQUIRED)
#   find_package(IMGUI REQUIRED)

#   include_directories(${SDL2_INCLUDE_DIRS})
#   include_directories(${VULAKN_INCLUDE_DIRS})
#   # include_directories(${IMGUI_INCLUDE_DIRS})
  
#   target_link_libraries(${PROJECT_NAME} ${VULAKN_LIBRARIES})
#   target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
#   # target_link_libraries(${PROJECT_NAME} ${IMGUI_LIBRARIES}})

#   set(CMAKE_THREAD_LIBS_INIT "-lpthread")
#   set(CMAKE_HAVE_THREADS_LIBRARY 1)
#   set(CMAKE_USE_WIN32_THREADS_INIT 0)
#   set(CMAKE_USE_PTHREADS_INIT 1)
#   set(THREADS_PREFER_PTHREAD_FLAG ON)
#   # Lets bundle it:
#   set_target_properties(${PROJECT_NAME} PROPERTIES
#      MACOSX_BUNDLE TRUE
#      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist
#   )

# elseif(LINUX)

#   #TODO Libs

#   find_package(SDL REQUIRED)
#   find_package(VULAKN REQUIRED)
#   find_package(IMGUI REQUIRED)

#   include_directories(${SDL_INCLUDE_DIRS})
#   include_directories(${VULAKN_INCLUDE_DIRS})
#   # include_directories(${IMGUI_INCLUDE_DIRS})
  
#   target_link_libraries(${PROJECT_NAME} ${VULAKN_LIBRARIES})
#   target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARIES})
#   # target_link_libraries(${PROJECT_NAME} ${IMGUI_LIBRARIES}})

#   set(CMAKE_THREAD_LIBS_INIT "-lpthread")
#   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
#   set(CMAKE_HAVE_THREADS_LIBRARY 1)
#   set(CMAKE_USE_WIN32_THREADS_INIT 0)
#   set(CMAKE_USE_PTHREADS_INIT 1)
#   set(THREADS_PREFER_PTHREAD_FLAG ON)
#   set(CMAKE_C_COMPILER gcc)
#   set(CMAKE_CXX_COMPILER g++)

# endif()