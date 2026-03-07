add_executable(opengl_triangle)

target_sources(opengl_triangle PRIVATE main.cpp)
target_link_libraries(opengl_triangle PRIVATE opengl SDL3::SDL3 stb::stb)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(RUNTIME_DIRS "$<TARGET_RUNTIME_DLLS:opengl_triangle>")
    add_custom_command(TARGET opengl_triangle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:opengl_triangle>
            $<TARGET_FILE_DIR:opengl_triangle>
        COMMAND_EXPAND_LISTS
    )
endif()
