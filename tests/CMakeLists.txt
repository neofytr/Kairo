add_executable(kairo_tests
    test_main.cpp
    test_math.cpp
    test_ecs.cpp
    test_physics.cpp
    test_events.cpp
    test_memory.cpp
    test_gameplay.cpp
    test_phase9.cpp
    test_phase11.cpp
    test_phase12.cpp
)

target_include_directories(kairo_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kairo_tests PRIVATE kairo_engine)
