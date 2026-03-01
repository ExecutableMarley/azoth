/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "CMemoryModule.hpp"

#include "../Types/EProcessArchitecture.hpp"
#include "../CProcess.hpp"

namespace Azoth
{


bool CMemoryModule::readPtr(Address addr, Address& out)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
    {
        if (uint32_t tmp; read(addr, tmp))
        {
            out = tmp;
            return true;
        }
        return false;
    }
    else
    {
        return read(addr, out);
    }
}

//=== Query Memory ===//

std::vector<MemoryRegion> CMemoryModule::queryAllMemoryRegions(Address startAddr, Address maxAddr, const MemoryRegionFilter &filter)
{
    std::vector<MemoryRegion> regionList;
    for (Address i = startAddr; i < maxAddr;)
    {
        MemoryRegion mr;
        if (queryMemory(i, mr))
        {
            if (mr.state == EMemoryState::Committed)
            {
                // Optionally filter regions
                if (!filter || filter(mr))
                {
                    regionList.push_back(mr);
                }
            }
            i = mr.baseAddress + mr.regionSize;
        }
        else
        {
            break;
        }
    }
    return regionList;
}

//Todo: Split this up.
bool CMemoryModule::virtualProtect(Address addr, size_t size, EMemoryProtection newProtect, EMemoryProtection *pOldProtection)
{
    if (size == 0 || addr + size < addr)
    {
        return setError(EPlatformError::InvalidArgument);
    }

    Address end = addr + size;

    auto it = _pageProtectRestoreMap.lower_bound(addr);

    bool isExactMatch = (it != _pageProtectRestoreMap.end() && it->first == addr);
    bool shouldDeleteEntry = false;
    bool shouldAddEntry = false;

    if (isExactMatch)
    {
        if (it->second.size != size)
        {
            return setError(EPlatformError::RestorationViolation);
        }

        if (it->second.origProtect == newProtect)
        {
            shouldDeleteEntry = true;
        }
    }
    else
    {
        // Forward overlap
        if (it != _pageProtectRestoreMap.end() && it->first < end)
        {
            return setError(EPlatformError::RestorationViolation);
        }

        // Backward overlap
        if (it != _pageProtectRestoreMap.begin())
        {
            auto prev = std::prev(it);
            if (prev->first + prev->second.size > addr)
            {
                return setError(EPlatformError::RestorationViolation);
            }
        }
        shouldAddEntry = true;
    }

    EMemoryProtection oldProt;
    if (!_platformLink->virtualProtect(addr, size, newProtect, &oldProt))
    {
        return false; // platform sets error
    }

    if (shouldDeleteEntry)
        _pageProtectRestoreMap.erase(it);
    else if (shouldAddEntry)
        addProtectExRestore(addr, size, oldProt);

    if (pOldProtection)
        *pOldProtection = oldProt;
    return setError(EPlatformError::Success);
}

bool CMemoryModule::restoreProtection(Address addr)
{
    auto it = _pageProtectRestoreMap.find(addr);
    if (it == _pageProtectRestoreMap.end())
        return true;

    const auto &entry = it->second;

    if (!_platformLink->virtualProtect(addr, entry.size, entry.origProtect, nullptr))
        return false;

    _pageProtectRestoreMap.erase(it);
    return true;
}

bool CMemoryModule::virtualFree(Address addr, bool allowUntracked)
{
    auto it = _allocRestoreMap.find(addr);
    if (it != _allocRestoreMap.end())
    {
        const bool success = _platformLink->virtualFree(addr);
        if (success)
        {
            _allocRestoreMap.erase(it);
        }
        return success;
    }

    if (allowUntracked)
    {
        return _platformLink->virtualFree(addr);
    }
    return setError(EPlatformError::InvalidArgument);
}

bool CMemoryModule::addMemoryRestorePoint(Address addr, size_t size)
{
    if (size == 0 || addr + size < addr)
        return setError(EPlatformError::InvalidArgument);

    Address end = addr + size;
    auto it = _codeMemoryRestoreMap.lower_bound(addr);

    // Exact match
    if (it != _codeMemoryRestoreMap.end() && it->first == addr)
    {
        if (it->second.size != size)
            return setError(EPlatformError::RestorationViolation);

        return setError(EPlatformError::Success);
    }

    // Forward overlap
    if (it != _codeMemoryRestoreMap.end() && it->first < end)
        return setError(EPlatformError::RestorationViolation);

    // Backward overlap
    if (it != _codeMemoryRestoreMap.begin())
    {
        auto prev = std::prev(it);
        if (prev->first + prev->second.size > addr)
            return setError(EPlatformError::RestorationViolation);
    }

    // Capture original bytes
    auto original = std::make_unique<uint8_t[]>(size);
    if (!read(addr, size, original.get()))
        return false;

    _codeMemoryRestoreMap.emplace(addr, CodeRestorePoint{size, std::move(original)});

    return setError(EPlatformError::Success);
}

bool CMemoryModule::patchReadOnlyMemory(Address addr, size_t size, const BYTE *buffer)
{
    MemoryRegion mr;
    if (!queryMemory(addr, mr))
        return false;

    EMemoryProtection tmpProtection = mr.protection | EMemoryProtection::Write;

    EMemoryProtection oldProtection;
    if (!virtualProtect(mr.baseAddress, mr.regionSize, tmpProtection, &oldProtection))
    {
        return false;
    }

    if (!addMemoryRestorePoint(addr, size))
    {
        // Todo: This can overwrite the last Error
        virtualProtect(mr.baseAddress, mr.regionSize, oldProtection, NULL);
        return false;
    }

    bool success = this->write(addr, size, buffer);

    // Todo: This also can overwrite the last Error
    virtualProtect(mr.baseAddress, mr.regionSize, oldProtection, NULL);

    return success;
}

bool CMemoryModule::restoreReadOnlyMemory(Address addr)
{
    auto it = _codeMemoryRestoreMap.find(addr);
    if (it == _codeMemoryRestoreMap.end())
        return true;

    const auto &entry = it->second;

    if (!write(addr, entry.size, entry.originalBytes.get()))
        return false;

    _codeMemoryRestoreMap.erase(it);
    return true;
}


}