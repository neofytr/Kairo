add_executable(sandbox main.cpp)

target_link_libraries(sandbox PRIVATE kairo_engine)

# copy assets to build directory so shaders can be found at runtime
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:sandbox>/assets
)
