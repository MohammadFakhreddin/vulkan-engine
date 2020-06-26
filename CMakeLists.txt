cmake_minimum_required (VERSION 3.0)

set(${PROJECT_NAME} "VULKAN_ENGINE")

project ("VULKAN_ENGINE" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CONFIGURATION_TYPES Debug;Release CACHE INTERNAL "" FORCE)
set (CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE INTERNAL "" FORCE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set (LIB_ROOT "${CMAKE_SOURCE_DIR}/dep/lib/win")
endif()

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out)
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out)

set(APP_SOURCE)
set(APP_HEADER)

# These files get included for build process
file(GLOB_RECURSE SRC_HEADER "src/*.h")
file(GLOB_RECURSE SRC_CPP "src/*.cpp")

file(GLOB_RECURSE LIB_HEADER "libs/include/*.h")
file(GLOB_RECURSE LIB_CPP "libs/include/*.cpp")

list(
  APPEND APP_SOURCE
  ${SRC_CPP}
  ${LIB_CPP}
)

list(
  APPEND APP_HEADER
  ${SRC_HEADER}
  ${LIB_HEADER}
)

# Includes these 2 directory for importing
include_directories ("src" "libs/include")
link_directories(${LIBRARY_OUTPUT_PATH} ${LIB_ROOT})
add_executable(${PROJECT_NAME} ${APP_HEADER} ${APP_SOURCE})