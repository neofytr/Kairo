file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(kairo_engine STATIC ${ENGINE_SOURCES})

target_include_directories(kairo_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/nlohmann
)

target_link_libraries(kairo_engine PUBLIC
    glfw
    glad
    stb
    OpenGL::GL
)
