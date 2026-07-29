add_library(Process STATIC)

target_sources(Process PRIVATE
    ProcessModules/CMemoryModule.cpp
    ProcessModules/CDecoderModule.cpp
    ProcessModules/CScannerModule.cpp
    Core/AddressCursor.cpp
    Core/MemoryCopy.cpp
    Core/MemoryBlock.cpp
    Core/PointerEndpoint.cpp
)

target_compile_definitions(Process PRIVATE ZYDIS_STATIC_BUILD)

target_sources(Process PRIVATE
    ThirdParty/Zydis/Zydis.c
)

target_compile_definitions(Process PUBLIC AZOTH_ENABLE_DECODER)

if (WIN32)
    target_sources(Process PRIVATE
        Platform/Windows/Utility/WinProcess.cpp
    )
elseif(UNIX AND NOT APPLE)
    target_sources(Process PRIVATE
        Platform/Linux/Utility/LinuxProcess.cpp
    )
endif()

target_include_directories(Process PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_compile_features(Process PUBLIC cxx_std_20)