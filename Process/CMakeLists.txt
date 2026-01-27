add_library(Process INTERFACE)

target_sources(Process INTERFACE
    ProcessModules/CMemoryModule.cpp
    ProcessModules/CDecoderModule.cpp
    ProcessModules/CScannerModule.cpp
    Core/MemoryCopy.cpp
    Core/MemoryBlock.cpp
    Core/PointerEndpoint.cpp
)

target_sources(Process INTERFACE
    ThirdParty/Zydis/Zydis.c
)

if (WIN32)
    target_sources(Process INTERFACE
        Platform/Windows/Utility/WinProcess.cpp
    )
elseif(UNIX AND NOT APPLE)
    target_sources(Process INTERFACE
        Platform/Linux/Utility/LinuxProcess.cpp
    )
endif()

target_include_directories(Process INTERFACE
    ${PROJECT_SOURCE_DIR}
)

target_compile_features(Process INTERFACE cxx_std_20)