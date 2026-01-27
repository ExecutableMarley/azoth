cmake_minimum_required(VERSION 3.16)
project(azoth_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Process)
add_subdirectory(tests)
add_subdirectory(examples)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sandbox/CMakeLists.txt")
    add_subdirectory(sandbox)
endif()