/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <utility>
#include <memory>

#undef UNICODE
#undef _UNICODE
#include <windows.h>

namespace Azoth
{


class SmartHandle 
{
public:
    explicit SmartHandle(HANDLE h = nullptr) : _handle(h) {}

    // Automatic cleanup
    ~SmartHandle() { close(); }

    // Disable copying
    SmartHandle(const SmartHandle&) = delete;
    SmartHandle& operator=(const SmartHandle&) = delete;

    // Enable moving
    SmartHandle(SmartHandle&& other) noexcept : _handle(other.release()) {}
    
    SmartHandle& operator=(SmartHandle&& other) noexcept
    {
        if (this != &other)
        {
            close();
            _handle = other.release();
        }
        return *this;
    }

    // Accessors
    HANDLE get() const { return _handle; }
    
    // Implicit Casting for convenience
    operator HANDLE() const { return _handle; }

    // Helper for "out" parameters
    HANDLE* address()
    {
        close();
        return &_handle;
    }

    bool isValid() const
    {
        return _handle != nullptr && _handle != INVALID_HANDLE_VALUE;
    }

    void close()
    {
        if (isValid())
        {
            CloseHandle(_handle);
            _handle = nullptr;
        }
    }

	// Retrieve Handle without auto closing
    HANDLE release()
    {
        return std::exchange(_handle, nullptr);
    }

private:
    HANDLE _handle = nullptr;
};

inline SmartHandle openProcessHandle(DWORD pid, DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ)
{
    return SmartHandle(OpenProcess(desiredAccess, FALSE, pid));
}


}