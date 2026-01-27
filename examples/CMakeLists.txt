add_executable(basic example_basic.cpp)
target_link_libraries(basic PRIVATE Process)

add_executable(custom_layer example_custom_layer.cpp)
target_link_libraries(custom_layer PRIVATE Process)

add_executable(memory1 example_memory1.cpp)
target_link_libraries(memory1 PRIVATE Process)

# IDE folder for (VS / VSCode CMake Tools)
set_target_properties(
    basic custom_layer memory1
    PROPERTIES FOLDER "examples"
)