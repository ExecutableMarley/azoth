cmake_minimum_required(VERSION 3.16)
project(azoth_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(examples)

enable_testing()

add_executable(azoth_tests)

target_sources(azoth_tests PRIVATE
    Process/ProcessModules/CMemoryModule.cpp
    Process/ProcessModules/CDecoderModule.cpp
    Process/ProcessModules/CScannerModule.cpp
    Process/Core/MemoryCopy.cpp
    Process/Core/PointerEndpoint.cpp
    tests/main.cpp
)

target_sources(azoth_tests PRIVATE
    Process/ThirdParty/Zydis/Zydis.c
)


if (WIN32)
    target_sources(azoth_tests PRIVATE
        Process/Platform/Windows/Utility/WinProcess.cpp
    )
endif()

target_include_directories(azoth_tests PRIVATE .)

# Register doctest binary
add_test(
    NAME azoth_tests
    COMMAND azoth_tests
)