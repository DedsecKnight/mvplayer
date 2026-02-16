add_executable(video_player)

target_sources(video_player PRIVATE main.cpp)
target_link_libraries(video_player PRIVATE mvplayer)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(RUNTIME_DIRS "$<TARGET_RUNTIME_DLLS:video_player>")
    add_custom_command(TARGET video_player POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:video_player>
            $<TARGET_FILE_DIR:video_player>
        COMMAND_EXPAND_LISTS
    )
endif()
