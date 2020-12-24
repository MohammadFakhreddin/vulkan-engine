cmake_minimum_required (VERSION 3.0)

set(${PROJECT_NAME} "VULKAN_ENGINE")

project ("VULKAN_ENGINE" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)
# TODO We need 2 solutions, 1 for Engine and 1 for test cases
set (CMAKE_CXX_STANDARD 17)
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

set(APP_SOURCE)
set(APP_HEADER)

# These files get included for build process
file(GLOB_RECURSE SRC_HEADER_H "src/*.h")
file(GLOB_RECURSE SRC_HEADER_HPP "src/*.hpp")
file(GLOB_RECURSE SRC_CPP "src/*.cpp")

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
include_directories ("src" "${CMAKE_SOURCE_DIR}/libs/include")
link_directories(${LIBRARY_OUTPUT_PATH} ${LIB_ROOT})
add_executable(${PROJECT_NAME} ${APP_HEADER} ${APP_SOURCE})

if(WINDOWS)

  find_package(SDL2 REQUIRED)
  find_package(VULKAN REQUIRED)
  find_package(IMGUI REQUIRED)

  include_directories(${SDL2_INCLUDE_DIRS})
  include_directories(${VULKAN_INCLUDE_DIRS})
  include_directories(${IMGUI_INCLUDE_DIRS})
  
  target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${IMGUI_LIBRARIES}})

elseif(MACOSX)

  find_package(SDL2 REQUIRED)
  find_package(VULKAN REQUIRED)
  find_package(IMGUI REQUIRED)

  include_directories(${SDL2_INCLUDE_DIRS})
  include_directories(${VULKAN_INCLUDE_DIRS})
  include_directories(${IMGUI_INCLUDE_DIRS})
  
  target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${IMGUI_LIBRARIES}})

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
  find_package(IMGUI REQUIRED)

  include_directories(${SDL_INCLUDE_DIRS})
  include_directories(${VULKAN_INCLUDE_DIRS})
  include_directories(${IMGUI_INCLUDE_DIRS})
  
  target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${IMGUI_LIBRARIES}})

  set(CMAKE_THREAD_LIBS_INIT "-lpthread")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(CMAKE_USE_WIN32_THREADS_INIT 0)
  set(CMAKE_USE_PTHREADS_INIT 1)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  set(CMAKE_C_COMPILER gcc)
  set(CMAKE_CXX_COMPILER g++)

endif()