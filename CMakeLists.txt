cmake_minimum_required (VERSION 3.0)

set(${PROJECT_NAME} "VULKAN_ENGINE")

project ("VULKAN_ENGINE" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CONFIGURATION_TYPES Debug;Release CACHE INTERNAL "" FORCE)
set (CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE INTERNAL "" FORCE)

add_definitions (/WX)           # Warnings as errors

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set (LIB_ROOT "${CMAKE_SOURCE_DIR}/libs/win")
endif()

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out)
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out)
# TODO It is better to have separate folders
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/out)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/out)

set(APP_SOURCE)
set(APP_HEADER)

# These files get included for build process
file(GLOB_RECURSE SRC_HEADER_H "src/*.h")
file(GLOB_RECURSE SRC_HEADER_HPP "src/*.hpp")
file(GLOB_RECURSE SRC_CPP "src/*.cpp")

# file(GLOB_RECURSE LIB_HEADER_H "libs/include/*.h")
# file(GLOB_RECURSE LIB_HEADER_HPP "libs/include/*.hpp")
# file(GLOB_RECURSE LIB_CPP "libs/include/*.cpp")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  file(GLOB VULKAN "libs/win/vulkan-1.lib")
  file(
    GLOB SDL 
    "libs/win/SDL2.lib" 
    "libs/win/SDL2main.lib"
  )
  list(
    APPEND COMPILED_LIBS 
    ${VULKAN}
    ${SDL}
  )
endif()

list(
  APPEND APP_SOURCE
  ${SRC_CPP}
)

list(
  APPEND APP_HEADER
  ${SRC_HEADER_H}
  ${SRC_HEADER_HPP}
)

# Includes these 2 directory for importing
include_directories ("src" "libs/include")
link_directories(${LIBRARY_OUTPUT_PATH} ${LIB_ROOT})
add_executable(${PROJECT_NAME} ${APP_HEADER} ${APP_SOURCE})
target_link_libraries(${PROJECT_NAME} ${COMPILED_LIBS})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    file(GLOB DLLS "libs/win/SDL2.dll" )
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        ${DLLS}      # <--this is in-file
        ${LIBRARY_OUTPUT_PATH}
    )       
endif()