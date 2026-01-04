cmake_minimum_required(VERSION 4.0)
project(video_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
include(${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)

find_package(ffmpeg REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenCV REQUIRED)
find_package(SDL3 REQUIRED CONFIG)

add_library(mvplayer)
add_library(multithreaded)

target_sources(
  mvplayer
  PRIVATE
    src/mvplayer/video_reader.cpp
    src/mvplayer/utils/conversion.cpp
    src/mvplayer/utils/owned.cpp
    src/mvplayer/frame_renderer.cpp

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

target_link_libraries(mvplayer PUBLIC ffmpeg::ffmpeg spdlog::spdlog opencv::opencv SDL3::SDL3)
target_link_libraries(multithreaded PUBLIC spdlog::spdlog)

add_subdirectory(apps)
add_subdirectory(tests)

