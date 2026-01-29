add_executable(azoth_tests)

target_sources(azoth_tests PRIVATE
    main.cpp
)

target_link_libraries(azoth_tests PRIVATE Process)

# Register doctest binary
add_test(
    NAME azoth_tests
    COMMAND azoth_tests
)