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

/*
Todo: Proper error management for these methods.
Consider implementing EProcessCapability enum  
*/

Address CScannerModule::findPatternEx(const MemoryRange& memRange, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
	size_t bufferSize = 0x1000;
    size_t patternSize = pattern.size();

    auto regions = _memory->queryAllMemoryRegions(memRange, filter);
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
                Address remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (memRange.contains(remoteAddress))
                    return remoteAddress;
            }
        }
    }
    return 0;
}

Address CScannerModule::findPatternEx(Address start, Address stop, const Pattern& pattern, const MemoryRegionFilter& filter)
{
	return findPatternEx(MemoryRange(start, stop), pattern, filter);
}

Address CScannerModule::findPatternEx(const Pattern& pattern, const MemoryRegionFilter& filter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return findPatternEx(MemoryRange::max_range_32bit(), pattern, filter);
    else
        return findPatternEx(MemoryRange::max_range_64bit(), pattern, filter);
}

Address CScannerModule::findPatternEx(std::span<const MemoryCopy> memSnap, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    const size_t patternSize = pattern.size();
    for (const auto& memCopy : memSnap)
    {
        const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
        for (const BYTE* curByte = memCopy.getBuffer(); curByte < (regionEnd - patternSize); curByte++)
        {
            if (pattern.matches(curByte, regionEnd))
            {
                return memCopy.translate(curByte);
            }
        }
    }
    return 0;
}

Address CScannerModule::findPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    return findPatternEx(std::span{ &memCopy, 1 }, pattern, filter);
}

std::vector<Address> CScannerModule::findAllPatternEx(const MemoryRange& memRange, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;
    size_t patternSize = pattern.size();

    std::vector<Address> results;

    auto regions = _memory->queryAllMemoryRegions(memRange, filter);
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
                Address remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (memRange.contains(remoteAddress))
                    results.push_back(remoteAddress);
            }
        }
    }
    return results;
}

std::vector<Address> CScannerModule::findAllPatternEx(Address start, Address stop, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    return findAllPatternEx(MemoryRange(start, stop), pattern, filter);
}

std::vector<Address> CScannerModule::findAllPatternEx(const Pattern& pattern, const MemoryRegionFilter& filter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return findAllPatternEx(MemoryRange::max_range_32bit(), pattern, filter);
    else
        return findAllPatternEx(MemoryRange::max_range_64bit(), pattern, filter);
}

std::vector<Address> CScannerModule::findAllPatternEx(std::span<const MemoryCopy> memSnap, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    std::vector<Address> results;

    const size_t patternSize = pattern.size();
    for (const auto& memCopy : memSnap)
    {
        const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
        for (BYTE* curByte = memCopy.getBuffer(); curByte < (regionEnd - patternSize); curByte++)
        {
            if (pattern.matches(curByte, regionEnd))
            {
                results.push_back(memCopy.translate(curByte));
            }
        }
    }
    return results;
}

std::vector<Address> CScannerModule::findAllPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    return findAllPatternEx(std::span{ &memCopy, 1 }, pattern, filter);
}

//--------------------------------------------------------
// 
//--------------------------------------------------------



PatternMatch CScannerModule::signatureScanEx(const MemoryRange& memRange, const Pattern& pattern)
{
    std::shared_ptr<BYTE[]> buffer = std::make_shared<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;
    size_t patternSize = pattern.size();

    auto regions = _memory->queryAllMemoryRegions(memRange, ProtectionFilter::require(EMemoryProtection::Execute));
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_shared<BYTE[]>(region.regionSize);
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

                if (!memRange.contains(ptr))
                    continue;

                return PatternMatch(ptr, curByte, buffer, region.regionSize);
            }
        }
    }
    return PatternMatch::Invalid();
}

PatternMatch CScannerModule::signatureScanEx(const Pattern& pattern)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return signatureScanEx(MemoryRange::max_range_32bit(), pattern);
    else
        return signatureScanEx(MemoryRange::max_range_64bit(), pattern);
}

PatternMatch CScannerModule::signatureScanEx(std::span<const MemoryCopy> memSnap, const Pattern& pattern)
{
    size_t patternSize = pattern.size();

    for (auto& memCopy : memSnap)
    {
        const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();

        for (BYTE *curByte = memCopy.getBuffer(); curByte < (regionEnd - patternSize); curByte++)
        {
            if (pattern.matches(curByte, regionEnd))
            {
                // Address in remote process
                uint64_t ptr = memCopy.translate(curByte);
                size_t offset = curByte - memCopy.getBuffer();

                // Consider: 64 Bytes would probably be enough already
                std::shared_ptr<BYTE[]> sharedBuffer = std::make_shared<BYTE[]>(memCopy.getSize());
                std::copy_n(memCopy.getBuffer(), memCopy.getSize(), sharedBuffer.get());
                return PatternMatch(ptr, sharedBuffer.get() + offset, sharedBuffer, memCopy.getSize());
            }
        }
    }
    return PatternMatch::Invalid();
}

PatternMatch CScannerModule::signatureScanEx(const MemoryCopy& memCopy, const Pattern& pattern)
{
    return signatureScanEx(std::span{ &memCopy, 1 }, pattern);
}

//--------------------------------------------------------
// 
//--------------------------------------------------------

Address CScannerModule::findNextValue(const MemoryRange& memRange, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(memRange, filter);
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
        for (BYTE* curByte = Address::alignUp(buffer.get(), alignment); curByte + valueSize < regionEnd; curByte += alignment)
        {
            if (memcmp(curByte, value, valueSize) == 0)
            {
                Address remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (memRange.contains(remoteAddress))
                    return remoteAddress;
            }
        }
    }
    return 0;
}

Address CScannerModule::findNextValue(Address start, Address stop, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    return findNextValue(MemoryRange(start, stop), value, valueSize, filter, alignment);
}

Address CScannerModule::findNextValue(const std::span<const MemoryCopy> memSnap, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    for (const auto &memCopy : memSnap)
    {
        if (!memCopy.valid())
            continue;

        const BYTE *regionEnd = memCopy.getBuffer() + memCopy.getSize();
        for (BYTE *curByte = Address::alignUp(memCopy.getBuffer(), alignment); curByte + valueSize < regionEnd; curByte += alignment)
        {
            if (memcmp(curByte, value, valueSize) == 0)
            {
                return memCopy.translate(curByte);
            }
        }
    }

    return 0;
}

Address CScannerModule::findNextValue(const MemoryCopy& memCopy, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    return findNextValue(std::span{ &memCopy, 1 }, value, valueSize, filter, alignment);
}

std::vector<Address> CScannerModule::findAllValues(const MemoryRange& memRange, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    std::vector<Address> results;
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(memRange, filter);
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
        for (BYTE* curByte = Address::alignUp(buffer.get(), alignment); curByte + valueSize < regionEnd; curByte += alignment)
        {
            if (memcmp(curByte, value, valueSize) == 0)
            {
                Address remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (memRange.contains(remoteAddress))
                    results.push_back(remoteAddress);
            }
        }
    }
    return results;
}

std::vector<Address> CScannerModule::findAllValues(Address start, Address stop, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    return findAllValues(MemoryRange(start, stop), value, valueSize, filter, alignment);
}

std::vector<Address> CScannerModule::findAllValues(const std::span<const MemoryCopy> memSnap, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    std::vector<Address> results;

    for (const auto &memCopy : memSnap)
    {
        const BYTE *regionEnd = memCopy.getBuffer() + memCopy.getSize();
        for (BYTE *curByte = Address::alignUp(memCopy.getBuffer(), alignment); curByte + valueSize < regionEnd; curByte += alignment)
        {
            if (memcmp(curByte, value, valueSize) == 0)
            {
                Address remoteAddress = memCopy.translate(curByte);
                results.push_back(remoteAddress);
            }
        }
    }

    return results;
}

std::vector<Address> CScannerModule::findAllValues(const MemoryCopy& memCopy, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter, size_t alignment)
{
    return findAllValues(std::span{ &memCopy, 1 }, value, valueSize, filter, alignment);
}

//--------------------------------------------------------
// 
//--------------------------------------------------------

std::vector<std::pair<Address, std::string>> CScannerModule::scanForStrings(const MemoryRange& memRange, size_t minSize, const MemoryRegionFilter& filter)
{
    std::vector<std::pair<Address, std::string>> results;

    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(memRange, filter);
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
                Address remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (memRange.contains(remoteAddress))
                {
                    results.emplace_back(remoteAddress, std::string(reinterpret_cast<const char*>(curByte), curStringLength));
                }
            }
            curByte += curStringLength;
        }
    }
    return results;
}

std::vector<std::pair<Address, std::string>> CScannerModule::scanForStrings(Address start, Address stop, size_t minSize, const MemoryRegionFilter& filter)
{
    return scanForStrings(MemoryRange(start, stop), minSize, filter);
}

std::vector<std::pair<Address, std::string>> CScannerModule::scanForStrings(size_t minSize, const MemoryRegionFilter& filter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return scanForStrings(MemoryRange::max_range_32bit(), minSize, filter);
    else
        return scanForStrings(MemoryRange::max_range_64bit(), minSize, filter);
}

std::vector<std::pair<Address, std::string>> CScannerModule::scanForStrings(const std::span<const MemoryCopy> memSnap, size_t minSize)
{
    std::vector<std::pair<Address, std::string>> results;

    for (const auto &memCopy : memSnap)
    {
        const BYTE *regionEnd = memCopy.getBuffer() + memCopy.getSize();
        for (BYTE *curByte = memCopy.getBuffer(); curByte + minSize < regionEnd; curByte++)
        {
            size_t curStringLength = MemIn::findAsciiStringLength(curByte, regionEnd - curByte);
            if (curStringLength >= minSize)
            {
                results.emplace_back(memCopy.translate(curByte), std::string(reinterpret_cast<const char *>(curByte), curStringLength));
            }
            curByte += curStringLength;
        }
    }

    return results;
}

std::vector<std::pair<Address, std::string>> CScannerModule::scanForStrings(const MemoryCopy& memCopy, size_t minSize)
{
    return scanForStrings(std::span{ &memCopy, 1 }, minSize);
}

std::vector<std::pair<Address, std::u16string>> CScannerModule::scanForWideStrings(const MemoryRange& memRange, size_t minSize, const MemoryRegionFilter& filter)
{
    std::vector<std::pair<Address, std::u16string>> results;

    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(memRange, ProtectionFilter::require(EMemoryProtection::Read));
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
            size_t curStringLength = MemIn::findAsciiStringUTF16Length(curByte, regionEnd - curByte) / sizeof(char16_t);
            if (curStringLength >= minSize)
            {
                Address remoteAddress = curByte - buffer.get() + region.baseAddress;
                if (memRange.contains(remoteAddress))
                {
                    results.emplace_back(remoteAddress, std::u16string(reinterpret_cast<const char16_t*>(curByte), curStringLength));
                }
            }

            curByte += curStringLength;
        }
    }
    return results;
}

std::vector<std::pair<Address, std::u16string>> CScannerModule::scanForWideStrings(Address start, Address stop, size_t minSize, const MemoryRegionFilter& filter)
{
    return scanForWideStrings(MemoryRange(start, stop), minSize, filter);
}

std::vector<std::pair<Address, std::u16string>> CScannerModule::scanForWideStrings(size_t minSize, const MemoryRegionFilter& filter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return scanForWideStrings(MemoryRange::max_range_32bit(), minSize, filter);
    else
        return scanForWideStrings(MemoryRange::max_range_64bit(), minSize, filter);
}

std::vector<std::pair<Address, std::u16string>> CScannerModule::scanForWideStrings(const std::span<const MemoryCopy> memSnap, size_t minSize)
{
    std::vector<std::pair<Address, std::u16string>> results;

    for (const auto &memCopy : memSnap)
    {
        const BYTE *regionEnd = memCopy.getBuffer() + memCopy.getSize();
        for (BYTE *curByte = memCopy.getBuffer(); curByte + minSize < regionEnd; curByte++)
        {
            size_t curStringLength = MemIn::findAsciiStringUTF16Length(curByte, regionEnd - curByte) / sizeof(char16_t);
            if (curStringLength >= minSize)
            {
                results.emplace_back(memCopy.translate(curByte), std::u16string((char16_t *)curByte, curStringLength));
            }
            curByte += curStringLength;
        }
    }

    return results;
}

std::vector<std::pair<Address, std::u16string>> CScannerModule::scanForWideStrings(const MemoryCopy& memCopy, size_t minSize)
{
    return scanForWideStrings(std::span{ &memCopy, 1 }, minSize);
}

Address CScannerModule::scanForCodeCave(const MemoryRange& memRange, size_t minSize, size_t alignment)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(memRange, ProtectionFilter::exact(EMemoryProtection::ReadExec));
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
        for (BYTE* curByte = Address::alignUp(buffer.get(), alignment); curByte + minSize < regionEnd; curByte += alignment)
        {
            if (*curByte == *(curByte + 1))
            {
                size_t codeCaveSize = MemIn::findCodeCaveSize(curByte, regionEnd - curByte);
                if (codeCaveSize > minSize)
                {
                    Address remoteAddress = curByte - buffer.get() + region.baseAddress;
                    if (memRange.contains(remoteAddress))
                        return remoteAddress;
                }
            }
        }
    }
    return 0;
}

Address CScannerModule::scanForCodeCave(Address start, Address stop, size_t minSize, size_t alignment)
{
    return scanForCodeCave(MemoryRange(start, stop), minSize, alignment);
}

Address CScannerModule::scanForCodeCave(size_t minSize, size_t alignment)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return scanForCodeCave(MemoryRange::max_range_32bit(), minSize, alignment);
    else
        return scanForCodeCave(MemoryRange::max_range_64bit(), minSize, alignment);
}

Address CScannerModule::scanForCodeCave(const std::span<const MemoryCopy> memSnap, size_t minSize, size_t alignment)
{
    for (const auto &memCopy : memSnap)
    {
        const BYTE *regionEnd = memCopy.getBuffer() + memCopy.getSize();
        for (BYTE *curByte = Address::alignUp(memCopy.getBuffer(), alignment); curByte + minSize < regionEnd; curByte += alignment)
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
    }

    return 0;
}

Address CScannerModule::scanForCodeCave(const MemoryCopy& memCopy, size_t minSize, size_t alignment)
{
    return scanForCodeCave(std::span{ &memCopy, 1 }, minSize, alignment);
}

std::vector<Address> CScannerModule::findAllCrossRefs(const ProcessImage& module, Address absoluteTargetAddress)
{
    if (!module.valid() || !((MemoryRange)module).contains(absoluteTargetAddress))
        return {};

    std::vector<Address> crossRefs;
    auto& decoder = this->_backPtr->getDecoder();
    //const Address absoluteTargetAddress = module.baseAddress + relativeTargetAddress;

    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;

    auto regions = _memory->queryAllMemoryRegions(module, ProtectionFilter::require(EMemoryProtection::Execute));
    for (const auto& region : regions)
    {
        if (region.regionSize > bufferSize)
        {
            buffer = std::make_unique<BYTE[]>(region.regionSize);
            bufferSize = region.regionSize;
        }

        if (!_memory->read(region.baseAddress, region.regionSize, buffer.get()))
            continue;

        DecoderCursor cursor{ buffer.get(), region.regionSize, region.baseAddress };
        Instruction decodedInstr;
        InstructionOperands operands;

        while (cursor)
        {
            if (!decoder.decodeNext(cursor, decodedInstr))
            {
                // Skip 1 byte and try again
                cursor.advance(1);
                continue;
            }

            if (!decodedInstr.mayReferenceAddress())
            {
                continue;
            }

            if (decoder.decodeOperands(decodedInstr, operands))
            {
                for (const auto& op : operands)
                {
                    Address addr = decodedInstr.getAbsoluteAddress(op);
                    if (addr == absoluteTargetAddress)
                        crossRefs.push_back(decodedInstr.addr());
                }
            }
        }
    }

    return crossRefs;
}

std::vector<Address> CScannerModule::findAllCrossRefs(std::span<const MemoryCopy> memSnap, Address absoluteTargetAddress)
{
    //Todo: We should consider storing vmemory mapping info inside memCopy

    std::vector<Address> crossRefs;
    auto& decoder = this->_backPtr->getDecoder();
    //const Address absoluteTargetAddress = 0 + relativeTargetAddress;

    for (const auto &memCopy : memSnap)
    {
        if (!memCopy.valid() || !((MemoryRange)memCopy).contains(absoluteTargetAddress))
            continue;

        DecoderCursor cursor{memCopy.getBuffer(), memCopy.getSize(), memCopy.getBaseAddress()};
        Instruction decodedInstr;
        InstructionOperands operands;

        while (cursor)
        {
            if (!decoder.decodeNext(cursor, decodedInstr))
            {
                // Skip 1 byte and try again
                cursor.advance(1);
                continue;
            }

            if (!decodedInstr.mayReferenceAddress())
            {
                continue;
            }

            if (decoder.decodeOperands(decodedInstr, operands))
            {
                for (const auto &op : operands)
                {
                    Address addr = decodedInstr.getAbsoluteAddress(op);
                    if (addr == absoluteTargetAddress)
                        crossRefs.push_back(decodedInstr.addr());
                }
            }
        }
    }
    return crossRefs;
}

std::vector<Address> CScannerModule::findAllCrossRefs(const MemoryCopy& memCopy, Address absoluteTargetAddress)
{
    return findAllCrossRefs(std::span{ &memCopy, 1 }, absoluteTargetAddress);
}

std::vector<Address> CScannerModule::findSymbolCrossRefs(const ProcessImage& module, const ImageSymbol& symbol)
{
    if (symbol.source == SymbolSource::Export && module.name == symbol.name)
    {
        return findAllCrossRefs(module, symbol.address);
    }
    else
    {
        //Get the import symbol, then scan for cross refs to that symbol
        ImageSymbol importSymbol;
        if (_backPtr->getSymbols().findSymbolByName(module.name, symbol.name, importSymbol))
        {
            assert( ((MemoryRange)module).contains(importSymbol.address) );
            return findAllCrossRefs(module, importSymbol.address);
        }
        else
            IPlatformLink::setError(EPlatformError::SymbolNotFound);
        return {};
    }
}

std::vector<Address> CScannerModule::findSymbolCrossRefs(const ProcessImage& module, std::span<const MemoryCopy> memSnap, const ImageSymbol& symbol)
{
    if (symbol.source == SymbolSource::Export && module.name == symbol.name)
    {
        return findAllCrossRefs(module, symbol.address);
    }
    else
    {
        //Get the import symbol, then scan for cross refs to that symbol
        ImageSymbol importSymbol;
        if (_backPtr->getSymbols().findSymbolByName(module.name, symbol.name, importSymbol))
        {
            assert( ((MemoryRange)module).contains(importSymbol.address) );
            return findAllCrossRefs(memSnap, importSymbol.address);
        }
        else
            IPlatformLink::setError(EPlatformError::SymbolNotFound);
        return {};
    }
}

std::vector<Address> CScannerModule::findSymbolCrossRefs(const ProcessImage& module, const MemoryCopy& memCopy, const ImageSymbol& symbol)
{
    return findSymbolCrossRefs(module, std::span{ &memCopy, 1 }, symbol);
}


}