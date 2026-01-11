add_executable(video_player)

target_sources(video_player PRIVATE main.cpp)
target_link_libraries(video_player PRIVATE mvplayer)
