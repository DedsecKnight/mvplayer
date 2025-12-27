cmake_minimum_required(VERSION 4.0)
project(video_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
include(${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)

find_package(ffmpeg REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenCV REQUIRED)
find_package(SDL3 REQUIRED CONFIG)

add_library(videoplayer)
target_sources(videoplayer
  PRIVATE
    src/player.cpp
    src/utils.cpp
    src/renderer.cpp

  PRIVATE
    FILE_SET vplayerPrivateHeaders
    TYPE HEADERS
    BASE_DIRS
      include
    FILES
      include/info.hpp

  PUBLIC
    FILE_SET vplayerHeaders
    TYPE HEADERS
    BASE_DIRS
      include
    FILES
      include/player.hpp
      include/utils.hpp
      include/renderer.hpp
)
target_link_libraries(videoplayer PUBLIC ffmpeg::ffmpeg spdlog::spdlog opencv::opencv SDL3::SDL3)

add_subdirectory(apps)

