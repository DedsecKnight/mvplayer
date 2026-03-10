cmake_minimum_required(VERSION 4.0)
project(video_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)

if(ENABLE_ASAN)
  set(SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")

  message(STATUS "Address Sanitizer enabled")
endif()

if(ENABLE_UBSAN)
  set(SANITIZER_FLAGS "-fsanitize=undefined -fno-omit-frame-pointer -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")

  message(STATUS "Undefined Behavior Sanitizer enabled")
endif()

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
include(${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)

find_package(ffmpeg REQUIRED)
find_package(spdlog REQUIRED)
find_package(SDL3 REQUIRED CONFIG)
find_package(CLI11 REQUIRED)
find_package(glad REQUIRED)
find_package(stb REQUIRED)

add_library(mvplayer SHARED)
add_library(multithreaded SHARED)
add_library(opengl SHARED)

target_sources(
  opengl
  PRIVATE
    src/opengl/shader.cpp
    src/opengl/vertex_buffer.cpp
    src/opengl/vertex_array.cpp
    src/opengl/texture.cpp
    src/opengl/index_buffer.cpp

  PUBLIC
    FILE_SET openglHeaders
    TYPE HEADERS
    BASE_DIRS
      include/opengl
    FILES
      include/opengl/shader.hpp
      include/opengl/vertex_array.hpp
      include/opengl/vertex_buffer.hpp
      include/opengl/texture.hpp
      include/opengl/index_buffer.hpp
)

target_sources(
  mvplayer
  PRIVATE
    src/mvplayer/video_reader.cpp
    src/mvplayer/media_context.cpp
    src/mvplayer/utils/conversion.cpp
    src/mvplayer/utils/owned.cpp
    src/mvplayer/frame_renderer.cpp
    src/mvplayer/audio_renderer.cpp
    src/mvplayer/frame_pool.cpp
    src/mvplayer/renderer/base.cpp
    src/mvplayer/renderer/rgb/renderer.cpp
    src/mvplayer/renderer/factory.cpp

  PRIVATE
    FILE_SET vplayerPrivateHeaders
    TYPE HEADERS
    BASE_DIRS
      include/mvplayer
    FILES
      include/mvplayer/info.hpp
      include/mvplayer/utils/owned.hpp

  PUBLIC
    FILE_SET vplayerHeaders
    TYPE HEADERS
    BASE_DIRS
      include/mvplayer
    FILES
      include/mvplayer/video_reader.hpp
      include/mvplayer/utils/conversion.hpp
      include/mvplayer/frame_renderer.hpp
)

target_sources(
  multithreaded
  PRIVATE
    src/multithreaded/engine/engine.cpp

  PUBLIC
    FILE_SET multithreadedHeaders
    TYPE HEADERS
    BASE_DIRS
      include/multithreaded
    FILES
      include/multithreaded/engine/engine.hpp
      include/multithreaded/utils/queue.hpp
)

target_compile_features(mvplayer PRIVATE cxx_std_23)
target_compile_features(multithreaded PRIVATE cxx_std_23)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set_target_properties(mvplayer PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  set_target_properties(multithreaded PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

target_link_libraries(opengl PUBLIC glad::glad)
target_link_libraries(multithreaded PUBLIC spdlog::spdlog)
target_link_libraries(mvplayer PUBLIC ffmpeg::ffmpeg spdlog::spdlog SDL3::SDL3 multithreaded CLI11::CLI11)

add_subdirectory(apps)
add_subdirectory(tests)

