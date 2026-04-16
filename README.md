# Azoth - Process Manipulation Library

[![CMake on multiple platforms](https://github.com/ExecutableMarley/azoth/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/ExecutableMarley/azoth/actions/workflows/cmake-multi-platform.yml)
[![License](https://img.shields.io/github/license/ExecutableMarley/geomath)](./LICENSE)

Azoth is a **platform-independent C++ library** for inspecting and manipulating processes and their memory.
It is primarily designed for **educational and research purposes**, with a strong focus on clean architecture, extensibility, and OS abstraction.

This project is a complete rewrite of my earlier library [CppMemoryHacker](https://github.com/ExecutableMarley/CppMemoryHacker), aiming for significantly improved
code quality, portability, and maintainability.

## Features

### Process & Memory Manipulation
- Read and write process memory
- Query memory regions and page attributes
- Allocate and free memory in processes
- Change memory protection flags
- Track memory modifications and restore original state

### Process Control
- Suspend and resume processes

### Scanning & Analysis
- Pattern / signature scanning
- String and value searching
- Retrieval of loaded process image data

### Architecture & Design
- Fully **OS-agnostic core**
- Platform-specific functionality abstracted via `IPlatformLink`
- Default platform implementations (Windows, Linux – WIP)
- Custom platform backends can be injected or fully replaced

### In Progress / Planned
- Decoder (incomplete)
- Encoder (planned)
- Thread manipulation
- Hooking utilities

## Integration

Azoth can be integrated either via **direct source inclusion** or as a **CMake subproject**.

### Option 1 - Direct Source Integration

#### Steps

1. Copy the `Process/` folder into your project  
2. Add all `.cpp` files to your build system  
3. Include the main interface:

```cpp
#include "Process/CProcess.hpp"
```

### Option 2 - CMake

If using CMake, Azoth can be added as a subdirectory.

#### Example
```txt
add_subdirectory(Process)
target_link_libraries(MyExecutable PRIVATE Process)
```

The **Process/** folder contains its own **CMakeLists.txt** which defines the logical Process library target.



## Example Usage

### Example 1: Generic Usage

```cpp
#include "CProcess.hpp"

using namespace Azoth;

int main()
{
    CProcess process(Platform::createDefaultLayer());

    // Initialize internal state
    if (!process.initialize())
        return -1;

    // Attach to the process
    if (!process.attach(Platform::getPID()))
        return -1;

    std::cout << "Attached to process\n";

    //Get Main ProcessImage
    auto mainImage = process.getProcessMainImage();
    std::cout << mainImage << "\n";

    // --- Memory Module ---
    // Provides read, write, allocate, protect, free and region enumeration
    auto& memory = process.getMemory();

    if (uint64_t someValue; memory.read(mainImage.baseAddress, &someValue))
    {
        std::cout << "Read value from main image: 0x" << someValue << "\n";
    }

    // --- Scanner Module ---
    // Pattern, signature and value scanning
    auto& scanner = process.getScanner();

    // Scan entire process memory
    Address globalMatch = scanner.findPatternEx(Pattern("FF FF FF FF ? ? ? ? FF FF"));
    if (globalMatch)
    {
        std::cout << "Pattern found globally at 0x" << globalMatch << "\n";
    }

    // Scan only the main module's memory region
    Address imageMatch  = scanner.findPatternEx(mainImage, Pattern("FF FF FF FF ? ? ? ? FF FF"));
    if (imageMatch)
    {
        std::cout << "Pattern found in main image at 0x" << imageMatch << "\n";
    }

    return 0;
}
```

### Example 2: Custom Platform Backends

```cpp
#include "Process/CProcess.hpp"
#include "Process/Platform/Windows/WinapiLink.hpp"

using namespace Azoth;

// Override a specific platform method from the default backend
class MySuperiorReadImplementation : public WinapiLink
{
    //Override Read method with your own code
    bool read(uint64_t addr, size_t size, void* buffer) const override
	{
        std::cout << "Custom read backend invoked!\n";

		size_t bytesRead;
		if (ReadProcessMemory(this->_hProcess, (LPVOID)addr, buffer, size, (SIZE_T*)&bytesRead) && bytesRead != 0)
			return true;

		return setError(EPlatformError::InternalError, GetLastError());
	}
};

// Alternatively, provide a completely custom platform backend
class MySuperiorImplementation : public IPlatformLink
{
    //.....
};

int main()
{
    // Using a partially overridden backend
    CProcess process1(std::make_unique<MySuperiorReadImplementation>());

    // Using a fully custom backend
    CProcess process2(std::make_unique<MySuperiorImplementation>());

    return 0;
}
```

## Status

TODO

## Disclaimer

Azoth is intended for educational, research, and legitimate software development purposes.

While the default interface does not expose inherently malicious functionality, the provided low-level process and memory manipulation can potentially be misused. 

The author does not endorse or support the use of this software for:
- Unauthorized access to systems or software
- Circumvention of security mechanisms
- Violation of software licenses or terms of service
- Any illegal or unethical activity
