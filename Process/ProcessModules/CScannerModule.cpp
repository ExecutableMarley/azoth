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

Address CScannerModule::findPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter)
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

std::vector<Address> CScannerModule:: findAllPatternEx(const Pattern& pattern, const MemoryRegionFilter& filter)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return findAllPatternEx(MemoryRange::max_range_32bit(), pattern, filter);
    else
        return findAllPatternEx(MemoryRange::max_range_64bit(), pattern, filter);
}

std::vector<Address> CScannerModule::findAllPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter)
{
    std::vector<Address> results;

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

Address CScannerModule::signatureScanEx(const MemoryRange& memRange, const Pattern& pattern, short type, int operatorIndex, int addressOffset)
{
    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(0x1000);
    size_t bufferSize = 0x1000;
    size_t patternSize = pattern.size();

    auto regions = _memory->queryAllMemoryRegions(memRange, ProtectionFilter::require(EMemoryProtection::Execute));
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

                if (!memRange.contains(ptr))
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
                    ptr -= memRange.startAddr;
                }
                return ptr + addressOffset;
            }
        }
    }
    return 0;
}

Address CScannerModule::signatureScanEx(Address start, Address stop, const Pattern& pattern, short type, int operatorIndex, int addressOffset)
{
    return signatureScanEx(MemoryRange(start, stop), pattern, type, operatorIndex, addressOffset);
}

Address CScannerModule::signatureScanEx(const Pattern& pattern, short type, int operatorIndex, int addressOffset)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return signatureScanEx(MemoryRange::max_range_32bit(), pattern, type, operatorIndex, addressOffset);
    else
        return signatureScanEx(MemoryRange::max_range_64bit(), pattern, type, operatorIndex, addressOffset);
}

Address CScannerModule::signatureScanEx(const MemoryCopy& memCopy, const Pattern& pattern, short type, int operatorIndex, int addressOffset)
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

Address CScannerModule::findNextValue(const MemoryRange& memRange, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter)
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

        //Todo: Check for alignment?
        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + valueSize < regionEnd; curByte++)
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

Address CScannerModule::findNextValue(Address start, Address stop, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter)
{
    return findNextValue(MemoryRange(start, stop), value, valueSize, filter);
}

Address CScannerModule::findNextValue(const MemoryCopy& memCopy, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter)
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

std::vector<Address> CScannerModule::findAllValues(const MemoryRange& memRange, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter)
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

        //Todo: Check for alignment?
        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + valueSize < regionEnd; curByte++)
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

std::vector<Address> CScannerModule::findAllValues(Address start, Address stop, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter)
{
    return findAllValues(MemoryRange(start, stop), value, valueSize, filter);
}

std::vector<Address> CScannerModule::findAllValues(const MemoryCopy& memCopy, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter)
{
    std::vector<Address> results;
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + valueSize < regionEnd; curByte++)
    {
        if (memcmp(curByte, value, valueSize) == 0)
        {
            Address remoteAddress = memCopy.translate(curByte);
            results.push_back(remoteAddress);
        }
    }
    return results;
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

std::vector<std::pair<Address, std::string>> CScannerModule::scanForStrings(const MemoryCopy& memCopy, size_t minSize)
{
    std::vector<std::pair<Address, std::string>> results;
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + minSize < regionEnd; curByte++)
    {
        size_t curStringLength = MemIn::findAsciiStringLength(curByte, regionEnd - curByte);
        if (curStringLength >= minSize)
        {
            results.emplace_back(memCopy.translate(curByte), std::string(reinterpret_cast<const char*>(curByte), curStringLength));
        }
        curByte += curStringLength;
    }
    return results;
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

std::vector<std::pair<Address, std::u16string>> CScannerModule::scanForWideStrings(const MemoryCopy& memCopy, size_t minSize)
{
    std::vector<std::pair<Address, std::u16string>> results;
    const BYTE* regionEnd = memCopy.getBuffer() + memCopy.getSize();
    for (BYTE* curByte = memCopy.getBuffer(); curByte + minSize < regionEnd; curByte++)
    {
        size_t curStringLength = MemIn::findAsciiStringUTF16Length(curByte, regionEnd - curByte) / sizeof(char16_t);
        if (curStringLength >= minSize)
        {
            results.emplace_back(memCopy.translate(curByte), std::u16string((char16_t*)curByte, curStringLength));
        }
        curByte += curStringLength;
    }
    return results;
}

Address CScannerModule::scanForCodeCave(const MemoryRange& memRange, size_t minSize)
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
        //Todo: Align curByte
        const BYTE* regionEnd = buffer.get() + region.regionSize;
        for (BYTE* curByte = buffer.get(); curByte + minSize < regionEnd; curByte += 4)
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

Address CScannerModule::scanForCodeCave(Address start, Address stop, size_t minSize)
{
    return scanForCodeCave(MemoryRange(start, stop), minSize);
}

Address CScannerModule::scanForCodeCave(size_t minSize)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
        return scanForCodeCave(MemoryRange::max_range_32bit(), minSize);
    else
        return scanForCodeCave(MemoryRange::max_range_64bit(), minSize);
}

Address CScannerModule::scanForCodeCave(const MemoryCopy& memCopy, size_t minSize)
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

std::vector<Address> CScannerModule::findAllCrossRefs(const ProcessImage& module, Address relativeTargetAddress)
{
    if (!module.valid() || relativeTargetAddress >= module.size)
        return {};

    std::vector<Address> crossRefs;

    auto& decoder = this->_backPtr->getDecoder();

    const Address absoluteTargetAddress = module.baseAddress + relativeTargetAddress;

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

        const BYTE* curByte = buffer.get();
        size_t remainingSize = region.regionSize;
        uint64_t runtimeAddress = region.baseAddress;
        //CompactInstruction instr;
        ZydisDecodedInstruction instr;
        InstructionOperands operands;

        while (decoder.decodeNext(curByte, remainingSize, runtimeAddress, instr))
        {
            bool hasPotentialAddress = (instr.raw.disp.size != 0) || 
                               (instr.raw.imm[0].size != 0) || 
                               (instr.raw.imm[1].size != 0);
            
            bool isRelative = (instr.attributes & ZYDIS_ATTRIB_IS_RELATIVE);

            if (!hasPotentialAddress && !isRelative)
            {
                continue;
            }

            if (decoder.decodeOperands(instr, operands))
            {
                for (size_t i = 0; i < operands.count; i++)
                {
                    if (operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY || operands[i].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                    {
                        Address addr = decoder.decodeAbsoluteMemoryAddress(instr, operands[i], runtimeAddress);
                        if (addr == absoluteTargetAddress)
                            crossRefs.push_back(runtimeAddress);
                    }
                }
            }
        }
    }

    return crossRefs;
}

std::vector<Address> CScannerModule::findAllCrossRefs(const MemoryCopy& memCopy, Address relativeTargetAddress)
{
    //Todo: This method is problematic since it won't work as expected on copies from a module
    //We should consider to store vmemory mapping info inside memCopy

    if (!memCopy.valid() || relativeTargetAddress >= memCopy.getSize())
        return {};

    const BYTE* curByte = memCopy.getBuffer();
    size_t remainingSize = memCopy.getSize();
    uint64_t runtimeAddress = memCopy.getBaseAddress();
    //CompactInstruction instr;
    ZydisDecodedInstruction instr;
    InstructionOperands operands;

    std::vector<Address> crossRefs;
    auto& decoder = this->_backPtr->getDecoder();
    const Address absoluteTargetAddress = memCopy.getBaseAddress() + relativeTargetAddress;

    while (decoder.decodeNext(curByte, remainingSize, runtimeAddress, instr))
    {
        bool hasPotentialAddress = (instr.raw.disp.size != 0) || 
                           (instr.raw.imm[0].size != 0) || 
                           (instr.raw.imm[1].size != 0);
        
        bool isRelative = (instr.attributes & ZYDIS_ATTRIB_IS_RELATIVE);
        if (!hasPotentialAddress && !isRelative)
        {
            continue;
        }
        if (decoder.decodeOperands(instr, operands))
        {
            for (size_t i = 0; i < operands.count; i++)
            {
                if (operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY || operands[i].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                {
                    Address addr = decoder.decodeAbsoluteMemoryAddress(instr, operands[i], runtimeAddress);
                    if (addr == absoluteTargetAddress)
                        crossRefs.push_back(runtimeAddress);
                }
            }
        }
    }
    
    return crossRefs;
}

}