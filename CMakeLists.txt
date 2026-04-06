cmake_minimum_required(VERSION 3.20)
project(Kairo VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# output binaries to a clean location
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- dependencies ---

# GLFW (system-installed)
find_package(glfw3 3.3 REQUIRED)

# GLAD (bundled)
add_library(glad STATIC
    external/glad/src/glad.c
)
target_include_directories(glad PUBLIC external/glad/include)

# stb (bundled, header-only with implementation in .cpp)
add_library(stb STATIC external/stb/stb_image_impl.cpp)
target_include_directories(stb PUBLIC external/stb)

# ImGui (bundled)
add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_impl_glfw.cpp
    external/imgui/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC external/imgui)
target_link_libraries(imgui PUBLIC glfw)

# miniaudio (bundled, single-header with implementation)
add_library(miniaudio STATIC external/miniaudio/miniaudio_impl.cpp)
target_include_directories(miniaudio PUBLIC external/miniaudio)
target_link_libraries(miniaudio PRIVATE dl pthread m)

# Lua 5.4 (bundled source)
file(GLOB LUA_SOURCES external/lua/*.c)
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC external/lua)
target_compile_definitions(lua PRIVATE LUA_USE_LINUX)

# OpenGL
find_package(OpenGL REQUIRED)

# --- engine library ---
add_subdirectory(engine)

# --- sandbox executable ---
add_subdirectory(sandbox)

# --- tests ---
add_subdirectory(tests)
