add_executable(dungeon main.cpp game.cpp)

target_link_libraries(dungeon PRIVATE kairo_engine)

add_custom_command(TARGET dungeon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:dungeon>/assets
)
