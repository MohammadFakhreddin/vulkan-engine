cmake_minimum_required (VERSION 3.0)

set(${PROJECT_NAME} "VULKAN_ENGINE")

project ("VULKAN_ENGINE" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CONFIGURATION_TYPES Debug;Release CACHE INTERNAL "" FORCE)
set (CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE INTERNAL "" FORCE)

if(WINDOWS)
  add_definitions (/WX)           # Warnings as errors
endif()

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

if (WINDOWS)
  set (LIB_ROOT "${CMAKE_SOURCE_DIR}/libs/win")
elseif(MACOSX)
  set (LIB_ROOT "libs/mac")
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

# if (WINDOWS)
#   file(GLOB VULKAN "{CMAKE_SOURCE_DIR}/libs/win/vulkan-1.lib")
#   file(
#     GLOB SDL 
#     "{CMAKE_SOURCE_DIR}/libs/win/SDL2.lib" 
#     "{CMAKE_SOURCE_DIR}/libs/win/SDL2main.lib"
#   )
#   list(
#     APPEND COMPILED_LIBS 
#     ${VULKAN}
#     ${SDL}
#   )
# elseif(MACOSX)
#   file(GLOB VULKAN "libs/win/libvulkan-1.dylib")
#   file(
#     GLOB SDL 
#     "libs/win/libSDL2.a" 
#     "libs/win/libSDL2main.a"
#   )
# endif()
list(
  APPEND APP_SOURCE
  ${SRC_CPP}
)

list(
  APPEND APP_HEADER
  ${SRC_HEADER_H}
  ${SRC_HEADER_HPP}
)

add_executable(${PROJECT_NAME} ${APP_HEADER} ${APP_SOURCE})

if(WINDOWS)
  find_package(SDL2 REQUIRED)
  find_package(VULKAN REQUIRED)
  include_directories(${SDL2_INCLUDE_DIRS})
  include_directories(${VULKAN_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
elseif(MACOSX)
  find_package(SDL2 REQUIRED)
  find_package(VULKAN REQUIRED)
  include_directories(${SDL2_INCLUDE_DIRS})
  include_directories(${VULKAN_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

  set(CMAKE_THREAD_LIBS_INIT "-lpthread")
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(CMAKE_USE_WIN32_THREADS_INIT 0)
  set(CMAKE_USE_PTHREADS_INIT 1)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  # Lets bundle it:
  set_target_properties(${PROJECT_NAME} PROPERTIES
     MACOSX_BUNDLE TRUE
     MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist
  )
elseif(LINUX)

  find_package(SDL REQUIRED)
  find_package(VULKAN REQUIRED)
  include_directories(${SDL_INCLUDE_DIRS})
  include_directories(${VULKAN_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARIES})

  set(CMAKE_THREAD_LIBS_INIT "-lpthread")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(CMAKE_USE_WIN32_THREADS_INIT 0)
  set(CMAKE_USE_PTHREADS_INIT 1)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  set(CMAKE_C_COMPILER gcc)
  set(CMAKE_CXX_COMPILER g++)

endif()

enable_testing()

find_package(GTest CONFIG REQUIRED)

# Includes these 2 directory for importing
# include_directories ("src" "libs/include")
# link_directories(${LIBRARY_OUTPUT_PATH} ${LIB_ROOT})
# target_link_libraries(${PROJECT_NAME} ${COMPILED_LIBS})

# if (WINDOWS)
#     file(GLOB DLLS "{CMAKE_SOURCE_DIR}/libs/win/SDL2.dll" )
#     add_custom_command(
#         TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
#         ${DLLS}      # <--this is in-file
#         ${LIBRARY_OUTPUT_PATH}
#     )       
# endif()
# file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH}/assets/shaders)
# Copying assets to output directory
# file(GLOB_RECURSE SHADERS "assets/shaders/*")
#add_custom_command(
#    TARGET ${PROJECT_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${SHADERS}
#    ${LIBRARY_OUTPUT_PATH}/assets/shaders
#)
# For future unit tests
# set(UNIT_TEST "unit-test")

# add_executable(${UNIT_TEST} ${TEST_SOURCE} ${TEST_HEADER})

# target_link_libraries(${UNIT_TEST} GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

# add_test("unit-test" ${UNIT_TEST})