# Azoth - Process Manipulation Library

Azoth is a **platform-independent C++ library** for inspecting and manipulating processes and their memory.
It is primarily designed for **educational and research purposes**, with a strong focus on clean architecture, extensibility, and OS abstraction.

This project is a complete rewrite of my earlier library **CppMemoryHacker**, aiming for significantly improved
code quality, portability, and maintainability.

## Features

TODO

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

The CProcess class provides the main high-level interface, while the platform layer defines how OS access is performed.


## Example Usage

TODO

## Status

TODO