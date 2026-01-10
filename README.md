# Azoth - Process Manipulation Library

Azoth is a **platform-independent C++ library** for inspecting and manipulating processes and their memory.
It is primarily designed for **educational and research purposes**, with a strong focus on clean architecture, extensibility, and OS abstraction.

This project is a complete rewrite of my earlier library **CppMemoryHacker**, aiming for significantly improved
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
- Access control system

## Integration

At the moment, Azoth is designed for **direct source integration**.

### Recommended Setup
1. Copy the `Process/` folder or the entire repository into your project
2. Add all `.cpp` files to your build system
3. Include the main interface:
```cpp
#include "Process/CProcess.hpp"
```
4. Include one of the default platform implementations or provide your own
```cpp
#include "Process/Platform/Windows/WinapiLink.hpp"
```

The **CProcess** class provides the main high-level interface, while the platform layer defines how OS access is performed.


## Example Usage

### Example 1: Generic Usage

```cpp
#include "Platform/Windows/WinapiLink.hpp"
#include "CProcess.hpp"

using namespace Azoth;

int main()
{
    WinapiLink platform;
    CProcess process(&platform);

    // Initialize internal state
    if (!process.initialize())
        return -1;

    // Attach to the process
    if (!process.attach(GetCurrentProcessId()))
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
    MySuperiorReadImplementation myLink1;
    CProcess process1(&myLink1);

    // Using a fully custom backend
    MySuperiorImplementation myLink2;
    CProcess process2(&myLink2);

    return 0;
}
```

## Status

TODO