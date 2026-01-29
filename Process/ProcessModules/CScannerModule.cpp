/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "CScannerModule.hpp"

#include "../CProcess.hpp"

#include "../Utility/MemIn.hpp"

#include <cstring>

namespace Azoth
{


CScannerModule::CScannerModule(CProcess* backPtr) : _backPtr(backPtr)
{
	this->_memory = &backPtr->getMemory();
}

uint64_t CScannerModule::findPatternEx(uint64_t start, uint64_t stop, const Pattern& pattern, ProtectionFilter protectionFilter)
{
	std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
	size_t bufferSize = 0x1000;
    size_t patternSize = pattern.size();

    auto regions = _memory->queryAllMemoryRegions(start, stop, protectionFilter.requireRead());
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!region.read())
            continue;

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        const BYTE* regionEnd = buffer.get() + region.regionSize;

        for (BYTE* curByte = buffer.get(); curByte < (regionEnd - patternSize); curByte++)
        {
            if (pattern.matches(curByte, regionEnd))
            {
                uint64_t remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (start <= remoteAddress && remoteAddress <= stop)
                    return remoteAddress;
            }
        }
    }
    return 0;
}

uint64_t CScannerModule::findPatternEx(const Pattern& pattern, ProtectionFilter protectionFilter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return findPatternEx(MemoryRange::max_range_32bit(), pattern, protectionFilter);
    else
        return findPatternEx(MemoryRange::max_range_64bit(), pattern, protectionFilter);
}

uint64_t CScannerModule::findPatternEx(const MemoryRange& memRange, const Pattern& pattern, ProtectionFilter protectionFilter)
{
    return findPatternEx(memRange.startAddr, memRange.stopAddr, pattern, protectionFilter);
}

uint64_t CScannerModule::findPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, ProtectionFilter protectionFilter)
{
    const size_t patternSize = pattern.size();
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (const BYTE* curByte = memCopy.getBuffer(); curByte < (regionEnd - patternSize); curByte++)
    {
        if (pattern.matches(curByte, regionEnd))
        {
            return memCopy.translate(curByte);
        }
    }
    return 0;
}

std::vector<uint64_t> CScannerModule::findAllPatternEx(uint64_t start, uint64_t stop, const Pattern& pattern, ProtectionFilter protectionFilter)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;
    size_t patternSize = pattern.size();

    std::vector<uint64_t> results;

    auto regions = _memory->queryAllMemoryRegions(start, stop, protectionFilter.requireRead());
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!region.read())
            continue;

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        const BYTE* regionEnd = buffer.get() + region.regionSize;

        for (BYTE* curByte = buffer.get(); curByte < (regionEnd - patternSize); curByte++)
        {
            if (pattern.matches(curByte, regionEnd))
            {
                uint64_t remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (start <= remoteAddress && remoteAddress <= stop)
                    results.push_back(remoteAddress);
            }
        }
    }
    return results;
}

std::vector<uint64_t> CScannerModule:: findAllPatternEx(const Pattern& pattern, ProtectionFilter protectionFilter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return findAllPatternEx(MemoryRange::max_range_32bit(), pattern, protectionFilter);
    else
        return findAllPatternEx(MemoryRange::max_range_64bit(), pattern, protectionFilter);
}

std::vector<uint64_t> CScannerModule::findAllPatternEx(const MemoryRange& memRange, const Pattern& pattern, ProtectionFilter protectionFilter)
{
    return findAllPatternEx(memRange.startAddr, memRange.stopAddr, pattern, protectionFilter);
}

std::vector<uint64_t> CScannerModule::findAllPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, ProtectionFilter protectionFilter)
{
    std::vector<uint64_t> results;

    const size_t patternSize = pattern.size();
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (const BYTE* curByte = memCopy.getBuffer(); curByte < (regionEnd - patternSize); curByte++)
    {
        if (pattern.matches(curByte, regionEnd))
        {
            results.push_back(memCopy.translate(curByte));
        }
    }
    return results;
}

//--------------------------------------------------------
// 
//--------------------------------------------------------

uint64_t CScannerModule::signatureScanEx(uint64_t start, uint64_t stop, const Pattern& pattern, short type, int operatorIndex, int addressOffset)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;
    size_t patternSize = pattern.size();

    uint64_t ptr = 0;
    auto regions = _memory->queryAllMemoryRegions(start, stop, ProtectionFilter::require(EMemoryProtection::Execute));
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        const BYTE* regionEnd = buffer.get() + region.regionSize;

        for (BYTE* curByte = buffer.get(); curByte < (regionEnd - patternSize); curByte++)
        {
            if (pattern.matches(curByte, regionEnd))
            {
                //Address in remote process
                uint64_t ptr = (uint64_t)curByte - (uint64_t)buffer.get() + (uint64_t)region.baseAddress;

                if (ptr < start || ptr >= stop)
                    continue;

                //Decode the address from Instruction Operand
                if (type & 1)
                {
                    ptr = _backPtr->getDecoder().decodeAbsoluteMemoryAddress(curByte, regionEnd - curByte, ptr, operatorIndex);
                    if (ptr == 0)
                        return 0;
                }
                //Calculate relative address
                if (type & 2)
                {
                    ptr -= start;
                }
                return ptr + addressOffset;
            }
        }
    }
    return 0;
}

uint64_t CScannerModule::signatureScanEx(const Pattern& pattern, short type, int operatorIndex, int addressOffset)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return signatureScanEx(MemoryRange::max_range_32bit(), pattern, type, operatorIndex, addressOffset);
    else
        return signatureScanEx(MemoryRange::max_range_64bit(), pattern, type, operatorIndex, addressOffset);
}

uint64_t CScannerModule::signatureScanEx(const MemoryRange& memRange, const Pattern& pattern, short type, int operatorIndex, int addressOffset)
{
    return signatureScanEx(memRange.startAddr, memRange.stopAddr, pattern, type, operatorIndex, addressOffset);
}

uint64_t CScannerModule::signatureScanEx(const MemoryCopy& memCopy, const Pattern& pattern, short type, int operatorIndex, int addressOffset)
{
    size_t patternSize = pattern.size();

    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();

    for (BYTE* curByte = memCopy.getBuffer(); curByte < (regionEnd - patternSize); curByte++)
    {
        if (pattern.matches(curByte, regionEnd))
        {
            //Address in remote process
            uint64_t ptr = memCopy.translate(curByte);

            //Decode the address from Instruction Operand
            if (type & 1)
            {
                ptr = _backPtr->getDecoder().decodeAbsoluteMemoryAddress(curByte, regionEnd - curByte, ptr, operatorIndex);
                if (ptr == 0)
                    return 0;
            }
            //Calculate relative address
            if (type & 2)
            {
                ptr -= memCopy.getBaseAddress();
            }
            return ptr + addressOffset;
        }
    }
    return 0;
}

//--------------------------------------------------------
// 
//--------------------------------------------------------

uint64_t CScannerModule::findNextValue(uint64_t start, uint64_t stop, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(start, stop, protectionFilter.requireRead());
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!region.read())
            continue;

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        //Todo: Check for alignment?
        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + valueSize < regionEnd; curByte++)
        {
            if (memcmp(curByte, value, valueSize) == 0)
            {
                uint64_t remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (start <= remoteAddress && remoteAddress <= stop)
                    return remoteAddress;
            }
        }
    }
    return 0;
}

uint64_t CScannerModule::findNextValue(MemoryRange& memRange, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter)
{
    return findNextValue(memRange.startAddr, memRange.stopAddr, value, valueSize, protectionFilter);
}

uint64_t CScannerModule::findNextValue(MemoryCopy& memCopy, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter)
{
    if (!memCopy.valid())
        return 0;

    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + valueSize < regionEnd; curByte++)
    {
        if (memcmp(curByte, value, valueSize) == 0)
        {
            return memCopy.translate(curByte);
        }
    }
    return 0;
}

std::vector<uint64_t> CScannerModule::findAllValues(uint64_t start, uint64_t stop, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter)
{
    std::vector<uint64_t> results;
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(start, stop, protectionFilter.requireRead());
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!region.read())
            continue;

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        //Todo: Check for alignment?
        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + valueSize < regionEnd; curByte++)
        {
            if (memcmp(curByte, value, valueSize) == 0)
            {
                uint64_t remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (start <= remoteAddress && remoteAddress <= stop)
                    results.push_back(remoteAddress);
            }
        }
    }
    return results;
}

std::vector<uint64_t> CScannerModule::findAllValues(MemoryRange& memRange, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter)
{
    return findAllValues(memRange.startAddr, memRange.stopAddr, value, valueSize, protectionFilter);
}

std::vector<uint64_t> CScannerModule::findAllValues(MemoryCopy& memCopy, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter)
{
    std::vector<uint64_t> results;
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + valueSize < regionEnd; curByte++)
    {
        if (memcmp(curByte, value, valueSize) == 0)
        {
            uint64_t remoteAddress = memCopy.translate(curByte);
            results.push_back(remoteAddress);
        }
    }
    return results;
}

//--------------------------------------------------------
// 
//--------------------------------------------------------

std::vector<std::pair<uint64_t, std::string>> CScannerModule::scanForStrings(uint64_t start, uint64_t stop, size_t minSize, ProtectionFilter protectionFilter)
{
    std::vector<std::pair<uint64_t, std::string>> results;

    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(start, stop, protectionFilter.requireRead());
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!region.read())
            continue;

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + minSize < regionEnd; curByte++)
        {
            size_t curStringLength = MemIn::findAsciiStringLength(curByte, regionEnd - curByte);

            if (curStringLength >= minSize)
            {
                //Todo: Check if between start stop
                std::string curString(reinterpret_cast<const char*>(curByte), curStringLength);
                results.push_back(std::pair<uint64_t, std::string>(curByte - buffer.get() + region.baseAddress, curString));
            }
            curByte += curStringLength;
        }
    }
    return results;
}

std::vector<std::pair<uint64_t, std::string>> CScannerModule::scanForStrings(const MemoryRange& memRange, size_t minSize, ProtectionFilter protectionFilter)
{
    return scanForStrings(memRange.startAddr, memRange.stopAddr, minSize, protectionFilter);
}

std::vector<std::pair<uint64_t, std::string>> CScannerModule::scanForStrings(size_t minSize, ProtectionFilter protectionFilter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return scanForStrings(MemoryRange::max_range_32bit(), minSize, protectionFilter);
    else
        return scanForStrings(MemoryRange::max_range_64bit(), minSize, protectionFilter);
}

std::vector<std::pair<uint64_t, std::string>> CScannerModule::scanForStrings(const MemoryCopy& memCopy, size_t minSize)
{
    std::vector<std::pair<uint64_t, std::string>> results;
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + minSize < regionEnd; curByte++)
    {
        size_t curStringLength = MemIn::findAsciiStringLength(curByte, regionEnd - curByte);
        if (curStringLength >= minSize)
        {
            std::string curString(reinterpret_cast<const char*>(curByte), curStringLength);
            results.push_back(std::pair<uint64_t, std::string>(memCopy.translate(curByte), curString));
        }
        curByte += curStringLength;
    }
    return results;
}

std::vector<std::pair<uint64_t, std::wstring>> CScannerModule::scanForWideStrings(uint64_t start, uint64_t stop, size_t minSize, ProtectionFilter protectionFilter)
{
    std::vector<std::pair<uint64_t, std::wstring>> results;

    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(start, stop, ProtectionFilter::require(EMemoryProtection::Read));
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!region.read())
            continue;

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + minSize < regionEnd; curByte++)
        {
            size_t curStringLength = MemIn::findAsciiStringUTF16Length(curByte, regionEnd - curByte);

            if (curStringLength >= minSize)
            {
                //Todo: Check if between start stop
                std::wstring curString((wchar_t*)curByte, curStringLength / 2);
                results.push_back(std::pair<uint64_t, std::wstring>(curByte - buffer.get() + region.baseAddress, curString));
            }

            curByte += curStringLength;
        }
    }
    return results;
}

std::vector<std::pair<uint64_t, std::wstring>> CScannerModule::scanForWideStrings(const MemoryRange& memRange, size_t minSize, ProtectionFilter protectionFilter)
{
    return scanForWideStrings(memRange.startAddr, memRange.stopAddr, minSize, protectionFilter);
}

std::vector<std::pair<uint64_t, std::wstring>> CScannerModule::scanForWideStrings(size_t minSize, ProtectionFilter protectionFilter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return scanForWideStrings(MemoryRange::max_range_32bit(), minSize, protectionFilter);
    else
        return scanForWideStrings(MemoryRange::max_range_64bit(), minSize, protectionFilter);
}

std::vector<std::pair<uint64_t, std::wstring>> CScannerModule::scanForWideStrings(const MemoryCopy& memCopy, size_t minSize)
{
    std::vector<std::pair<uint64_t, std::wstring>> results;
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + minSize < regionEnd; curByte++)
    {
        size_t curStringLength = MemIn::findAsciiStringUTF16Length(curByte, regionEnd - curByte);
        if (curStringLength >= minSize)
        {
            std::wstring curString((wchar_t*)curByte, curStringLength / 2);
            results.push_back(std::pair<uint64_t, std::wstring>(memCopy.translate(curByte), curString));
        }
        curByte += curStringLength;
    }
    return results;
}

uint64_t CScannerModule::scanForCodeCave(uint64_t start, uint64_t stop, size_t minSize)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(start, stop, ProtectionFilter::exact(EMemoryProtection::ReadExec));
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;
        //Todo: Align curByte
        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + minSize < regionEnd; curByte += 4)
        {
            if (*curByte == *(curByte + 1))
            {
                size_t codeCaveSize = MemIn::findCodeCaveSize(curByte, regionEnd - curByte);
                if (codeCaveSize > minSize)
                {
                    //Todo: Check if between start stop
                    return curByte - buffer.get() + region.baseAddress;
                }
            }
        }
    }
    return 0;
}

uint64_t CScannerModule::scanForCodeCave(size_t minSize)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return scanForCodeCave(MemoryRange::max_range_32bit(), minSize);
    else
        return scanForCodeCave(MemoryRange::max_range_64bit(), minSize);
}

uint64_t CScannerModule::scanForCodeCave(const MemoryRange& memRange, size_t minSize)
{
    return scanForCodeCave(memRange.startAddr, memRange.stopAddr, minSize);
}


uint64_t CScannerModule::scanForCodeCave(const MemoryCopy& memCopy, size_t minSize)
{
    //Todo: Align curByte
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + minSize < regionEnd; curByte += 4)
    {
        if (*curByte == *(curByte + 1))
        {
            size_t codeCaveSize = MemIn::findCodeCaveSize(curByte, regionEnd - curByte);
            if (codeCaveSize > minSize)
            {
                return memCopy.translate(curByte);
            }
        }
    }
    return 0;
}


}