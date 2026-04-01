/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#if __linux__

#include "../../../Types/EMemoryProtection.hpp"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace Azoth
{


inline bool waitForStop(pid_t pid)
{
    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
        return false;

    return WIFSTOPPED(status);
}

/**
 * @internal
 * @brief RAII guard for ptrace register backup and restore.
 */
class RegisterGuard
{
public:
    explicit RegisterGuard(pid_t pid) : _pid(pid), _valid(false)
    {
        if (ptrace(PTRACE_GETREGS, _pid, nullptr, &_saved) == 0)
            _valid = true;
    }

    ~RegisterGuard()
    {
        if (_valid)
            ptrace(PTRACE_SETREGS, _pid, nullptr, &_saved);
    }

    user_regs_struct& regs() { return _saved; }

private:
    pid_t _pid;
    user_regs_struct _saved{};
    bool _valid;
};

/**
 * @internal
 * @brief RAII guard for patching code in the target process.
 *
 * This class writes a small patch (syscall; int3) at the specified address
 * and restores the original bytes when destroyed. It is designed to be used
 * in conjunction with ptrace to perform remote syscalls without permanently modifying the target process's code.
 */
class CodePatchGuard
{
public:
    CodePatchGuard(pid_t pid, std::uint64_t addr)
        : _pid(pid), _addr(addr), _size(0)
    {
        // syscall; int3
        const std::uint8_t patch[] = { 0x0f, 0x05, 0xcc };
        _size = sizeof(patch);

        // Backup original bytes
        for (size_t i = 0; i < _size; i += sizeof(long))
        {
            long data = ptrace(PTRACE_PEEKTEXT, _pid, _addr + i, nullptr);
            std::memcpy(_backup + i, &data, sizeof(long));
        }

        // Write patch
        for (size_t i = 0; i < _size; i += sizeof(long))
        {
            long chunk = 0;
            size_t copy = (_size - i) < sizeof(long) ? (_size - i) : sizeof(long);
            std::memset(&chunk, 0, sizeof(long));
            std::memcpy(&chunk, patch + i, copy);
            ptrace(PTRACE_POKETEXT, _pid, _addr + i, chunk);
        }
    }

    ~CodePatchGuard()
    {
        // Restore original bytes
        for (size_t i = 0; i < _size; i += sizeof(long))
        {
            long chunk = 0;
            std::memcpy(&chunk, _backup + i, sizeof(long));
            ptrace(PTRACE_POKETEXT, _pid, _addr + i, chunk);
        }
    }

private:
    pid_t _pid;
    std::uint64_t _addr;
    std::uint8_t _backup[16]{};
    size_t _size;
};

struct SyscallResult
{
    long value;   // return value (valid if success)
    int error;    // errno (0 if success)
};

/**
 * @internal
 * @brief Helper class for managing a ptrace session with a target process.
 *
 * This class provides methods to attach to a process, perform remote syscalls,
 * and automatically detach when destroyed. It is designed to be used for performing
 * operations on the target process's memory and state without requiring permanent code modifications.
 */
class PtraceSession
{
public:
    explicit PtraceSession(pid_t pid) : _pid(pid), _attached(false) {}

    ~PtraceSession()
    {
        if (_attached)
            detach();
    }

    bool attach()
    {
        if (ptrace(PTRACE_ATTACH, _pid, nullptr, nullptr) != 0)
            return false;

        if (!waitForStop(_pid))
            return false;

        _attached = true;
        return true;
    }

    void detach()
    {
        ptrace(PTRACE_DETACH, _pid, nullptr, nullptr);
        _attached = false;
    }

    SyscallResult remoteSyscall(long syscall,
                       long arg1 = 0, long arg2 = 0, long arg3 = 0,
                       long arg4 = 0, long arg5 = 0, long arg6 = 0)
    {
        RegisterGuard regs(_pid);
        user_regs_struct r = regs.regs();

        std::uint64_t rip = r.rip;

        CodePatchGuard patch(_pid, rip);

        // Setup registers (x86_64 ABI)
        r.rax = syscall;
        r.rdi = arg1;
        r.rsi = arg2;
        r.rdx = arg3;
        r.r10 = arg4;
        r.r8  = arg5;
        r.r9  = arg6;

        if (ptrace(PTRACE_SETREGS, _pid, nullptr, &r) != 0)
            throw std::runtime_error("SETREGS failed");

        // Continue until int3
        if (ptrace(PTRACE_CONT, _pid, nullptr, nullptr) != 0)
            throw std::runtime_error("CONT failed");

        int status = 0;
        waitpid(_pid, &status, 0);

        if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGTRAP)
            throw std::runtime_error("Did not hit syscall trap");

        // Read result
        if (ptrace(PTRACE_GETREGS, _pid, nullptr, &r) != 0)
            throw std::runtime_error("GETREGS failed");

        SyscallResult res{};

        if ((long)r.rax < 0)
        {
            res.value = -1;
            res.error = -(long)r.rax;
        }
        else
        {
            res.value = (long)r.rax;
            res.error = 0;
        }
        return res;
        //return static_cast<long>(r.rax);
    }

private:
    pid_t _pid;
    bool _attached;
};


}


#endif