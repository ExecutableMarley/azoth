/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#ifdef _WIN32

#include "../../IPlatformLink.hpp"
#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <tlhelp32.h>

namespace Azoth
{


/**
 * @brief Convert EMemoryProtection flags to Windows PAGE_* constants.
 *
 * Translates an abstract EMemoryProtection value into the closest matching
 * Windows page protection flag suitable for VirtualAlloc / VirtualProtect.
 */
inline DWORD toWinProtect(EMemoryProtection prot)
{
    switch (prot)
    {
    case EMemoryProtection::None:      return PAGE_NOACCESS;
    case EMemoryProtection::Read:      return PAGE_READONLY;
    case EMemoryProtection::Write:     return PAGE_READWRITE;
    case EMemoryProtection::Execute:   return PAGE_EXECUTE;
    case EMemoryProtection::ReadWrite: return PAGE_READWRITE;
    case EMemoryProtection::ReadExec:  return PAGE_EXECUTE_READ;
    case EMemoryProtection::RWX:       return PAGE_EXECUTE_READWRITE;
    default: return PAGE_NOACCESS;
    }
}

/**
 * @brief Convert Windows PAGE_* protection flags to EMemoryProtection flags.
 *
 * @note Some Windows protection flags collapse into the same abstract
 *       representation. Information such as guard or write-combine flags
 *       is intentionally ignored.
 */
inline EMemoryProtection fromWinProtect(DWORD prot)
{
    if (prot & (PAGE_NOACCESS))          return EMemoryProtection::None;
    if (prot & (PAGE_READONLY))          return EMemoryProtection::Read;
    if (prot & (PAGE_READWRITE))         return EMemoryProtection::ReadWrite;
    if (prot & (PAGE_EXECUTE))           return EMemoryProtection::Execute;
    if (prot & (PAGE_EXECUTE_READ))      return EMemoryProtection::ReadExec;
    if (prot & (PAGE_EXECUTE_READWRITE)) return EMemoryProtection::RWX;
    return EMemoryProtection::None;
}

/**
 * @brief Convert EMemoryState to Windows MEM_* constants.
 *
 * Translates EMemoryState values to their Windows equivalents as used by
 * VirtualQuery and related APIs.
 */
inline DWORD toWinState(EMemoryState state)
{
    switch (state)
    {
    case EMemoryState::Unknown:   return 0;
    case EMemoryState::Free:      return MEM_FREE;
    case EMemoryState::Reserved:  return MEM_RESERVE;
    case EMemoryState::Committed: return MEM_COMMIT;
    default: return 0;
    }
}

/**
 * @brief Convert Windows MEM_* allocation state to EMemoryState.
 */
inline EMemoryState fromWinState(DWORD state)
{
    if (state == MEM_COMMIT)  return EMemoryState::Committed;
    if (state == MEM_RESERVE) return EMemoryState::Reserved;
    if (state == MEM_FREE)    return EMemoryState::Free;

    return EMemoryState::Unknown;
}

/**
 * @brief Create a ProcessImage Object from a MODULEENTRY32 structure.
 */
inline ProcessImage fromWinModule(const MODULEENTRY32& entry)
{
    return ProcessImage(
        (uint64_t)entry.hModule,
        entry.modBaseSize,
        std::string(entry.szModule),
        std::string(entry.szExePath)
    );
}

/**
 * @brief Create a MemoryRegion from MEMORY_BASIC_INFORMATION.
 */
inline MemoryRegion fromWinBasicInformation(const MEMORY_BASIC_INFORMATION& mbi)
{
    return MemoryRegion(
        (uint64_t)mbi.BaseAddress,
        mbi.RegionSize,
        fromWinProtect(mbi.Protect),
        fromWinState(mbi.State),
        (EMemoryType)mbi.Type
    );
}


}

#endif