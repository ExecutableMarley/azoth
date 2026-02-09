add_executable(azoth_tests)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "test_*.cpp")

target_sources(azoth_tests PRIVATE
    main.cpp
    ${TEST_SOURCES}
)

target_link_libraries(azoth_tests PRIVATE Process)

# Register doctest binary
add_test(
    NAME azoth_tests
    COMMAND azoth_tests
)