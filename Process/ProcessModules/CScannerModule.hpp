/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../Types/Address.hpp"
#include "../Types/MemoryRange.hpp"
#include "../Types/Pattern.hpp"

#include "../Core/ProcessImage.hpp"
#include "../Core/MemoryRegion.hpp"
#include "../Core/MemoryCopy.hpp"

#include <span>
#include <vector>

namespace Azoth
{


class CProcess;
class CMemoryModule;

/**
 * @brief Represents a pattern scan match with utilities for address resolution.
 */
class PatternMatch
{
	/*
	Experimental class to easily allow the modification of pattern scan results.
	*/
public:
    using Buffer = BYTE[];

    friend class CScannerModule;

private:
    Address _address;
    BYTE*   _curByte;
    std::shared_ptr<Buffer> _sharedBuffer;
    size_t _regionSize;
	bool _invalid;

    PatternMatch(Address addr, BYTE* cur, std::shared_ptr<Buffer> buf, size_t regionSize, bool invalid = false) 
        : _address(addr), _curByte(cur), _sharedBuffer(buf), _regionSize(regionSize), _invalid(invalid) {}
    
    bool insideBuffer(BYTE* ptr, size_t size) const
    {
        if (!_sharedBuffer) return false;
        BYTE* start = _sharedBuffer.get();
        BYTE* end = start + _regionSize;
        return (start <= ptr && ptr + size < end);
    }

public:
    static PatternMatch Invalid() {
        return PatternMatch(0, nullptr, nullptr, 0, true);
    }

    Address get() const { return _address; }

    PatternMatch add(uint64_t offset) const {
		if (_invalid) Invalid();
        return PatternMatch(_address + offset, _curByte + offset, _sharedBuffer, _regionSize); 
    }
    
    PatternMatch sub(uint64_t offset) const {
		if (_invalid) Invalid();
        return PatternMatch(_address - offset, _curByte - offset, _sharedBuffer, _regionSize);
    }

    PatternMatch read4(uint64_t offset = 0) const {
        if (!insideBuffer(_curByte + offset, 4)) return Invalid();
        Address absAddr = *(uint32_t*)(_curByte + offset);
        return PatternMatch(absAddr, nullptr, nullptr, 0);
    }

    PatternMatch read8(uint64_t offset = 0) const {
        if (!insideBuffer(_curByte + offset, 8)) return Invalid();
        Address absAddr = *(uint64_t*)(_curByte + offset);
        return PatternMatch(absAddr, nullptr, nullptr, 0);
    }

    PatternMatch readRel(uint64_t offset = 0) const {
        if (!insideBuffer(_curByte + offset, 4)) return Invalid();
        uint32_t relAddr = *(uint32_t*)(_curByte + offset);
        return PatternMatch(_address + relAddr, _curByte + relAddr, _sharedBuffer, _regionSize);
    }
};


/**
 * @class CScannerModule
 * @brief Provides memory scanning and pattern search utilities over a process address space.
 *
 * This module specializes in iterating over memory ranges of a target process and locating
 * specific byte patterns, values, strings, or structural signatures.
 */
class CScannerModule
{
public:
	CScannerModule(CProcess* backPtr);

public:
	CScannerModule(const CScannerModule&) = delete;

    CScannerModule& operator=(const CScannerModule&) = delete;

public:
	//--------------------------------------------------------
	// Pattern scanning
	//--------------------------------------------------------

	/**
	 * @brief Finds the first occurrence of a byte pattern within a memory range.
	 *
	 * @return Address of the first match, or 0 if none was found.
	 */
	Address findPatternEx(const MemoryRange& memRange, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	Address findPatternEx(Address start, Address stop, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	Address findPatternEx(const Pattern& pattern, const MemoryRegionFilter& filter = {});

	Address findPatternEx(std::span<const MemoryCopy> memSnap, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	Address findPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	/**
	 * @brief Finds all occurrences of a byte pattern within a memory range.
	 *
	 * @return A list of absolute addresses where the pattern was found.
	 */
	std::vector<Address> findAllPatternEx(const MemoryRange& memRange, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	std::vector<Address> findAllPatternEx(Address start, Address stop, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	std::vector<Address> findAllPatternEx(const Pattern& pattern, const MemoryRegionFilter& filter = {});

	std::vector<Address> findAllPatternEx(std::span<const MemoryCopy> memSnap, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	std::vector<Address> findAllPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	//--------------------------------------------------------
	// Signature-based scanning
	//--------------------------------------------------------


	/**
	 * Todo: update this
	 * @brief Performs a signature scan and resolves a derived address.
	 *
	 * This function extends pattern scanning by applying additional logic
	 * (e.g. instruction decoding or operand resolution) to compute a final address.
	 *
	 * Typical use cases include locating RIP-relative addresses, call/jump targets,
	 * or embedded pointers inside instruction streams.
	 *
	 * @param type           1 for Operand resolution, 2 for relative return address, 1 | 2 for both
	 * @param operatorIndex  Index of the operand used for address resolution.
	 * @param addressOffset  Additional offset applied to the resolved address.
	 *
	 * @return The resolved address, or 0 if the signature was not found or invalid.
	 */
	PatternMatch signatureScanEx(const MemoryRange& memRange, const Pattern& pattern);

	PatternMatch signatureScanEx(const Pattern& pattern);

	PatternMatch signatureScanEx(std::span<const MemoryCopy> memSnap, const Pattern& pattern);

	PatternMatch signatureScanEx(const MemoryCopy& memCopy, const Pattern& pattern);

	//--------------------------------------------------------
	// Value scanning
	//--------------------------------------------------------

	/**
	 * @brief Finds the next occurrence of a raw value in memory.
	 *
	 * @return Address of the first occurrence after @p start, or 0 if not found.
	 */
	Address findNextValue(const MemoryRange& memRange, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	Address findNextValue(Address start, Address stop, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	Address findNextValue(std::span<const MemoryCopy> memSnap, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	Address findNextValue(const MemoryCopy& memCopy, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	/**
	 * @brief Finds the next occurrence of a raw value in memory.
	 *
	 * Type-safe wrapper around findNextValue().
	 * 
	 * @note Performs a byte-wise comparison of the provided value.
	 * 
	 * @return Address of the first occurrence after @p start, or 0 if not found.
	 */
	template <typename T>
	Address findNextValue(const MemoryRange& memRange, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findNextValue(memRange, (BYTE*)&value, sizeof(value), filter);
	}

	template <typename T>
	Address findNextValue(Address start, Address stop, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findNextValue(start, stop, (BYTE*)&value, sizeof(value), filter);
	}

	template <typename T>
	Address findNextValue(std::span<const MemoryCopy> memSnap, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findNextValue(memSnap, (BYTE*)&value, sizeof(value), filter);
	}

	template <typename T>
	Address findNextValue(const MemoryCopy& memCopy, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findNextValue(memCopy, (BYTE*)&value, sizeof(value), filter);
	}

	/**
	 * @brief Finds all occurrences of a raw value in memory.
	 * 
	 * @note Performs a byte-wise comparison of the provided value.
	 * 
	 * @return A list of addresses where the value was found.
	 */
	std::vector<Address> findAllValues(const MemoryRange& memRange, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	std::vector<Address> findAllValues(Address start, Address stop, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	std::vector<Address> findAllValues(std::span<const MemoryCopy> memSnap, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	std::vector<Address> findAllValues(const MemoryCopy& memCopy, BYTE* value, size_t valueSize, const MemoryRegionFilter& filter = {}, size_t alignment = 1);

	/**
	 * @brief Finds all occurrences of a raw value in memory.
	 * 
	 * Type-safe wrapper around findNextValue().
	 * 
	 * @note Performs a byte-wise comparison of the provided value.
	 * 
	 * @return A list of addresses where the value was found.
	 */
	template <typename T>
	std::vector<Address> findAllValues(const MemoryRange& memRange, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findAllValues(memRange, (BYTE*)&value, sizeof(value), filter);
	}

	template <typename T>
	std::vector<Address> findAllValues(Address start, Address stop, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findAllValues(start, stop, (BYTE*)&value, sizeof(value), filter);
	}

	template <typename T>
	std::vector<Address> findAllValues(std::span<const MemoryCopy> memSnap, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findAllValues(memSnap, (BYTE*)&value, sizeof(value), filter);
	}

	template <typename T>
	std::vector<Address> findAllValues(const MemoryCopy& memCopy, T value, const MemoryRegionFilter& filter = {}, size_t alignment = 1)
	{
		return findAllValues(memCopy, (BYTE*)&value, sizeof(value), filter);
	}

	//--------------------------------------------------------
	// Specialized scans
	//--------------------------------------------------------

	/**
	 * @brief Scans memory for ASCII strings.
	 *
	 * Identifies contiguous printable character sequences with a minimum length.
	 *
	 * @return A list of (address, string) pairs.
	 */
	std::vector<std::pair<Address, std::string>> scanForStrings(const MemoryRange& memRange, size_t minSize, const MemoryRegionFilter& filter = {});

	std::vector<std::pair<Address, std::string>> scanForStrings(Address start, Address stop, size_t minSize, const MemoryRegionFilter& filter = {});

	std::vector<std::pair<Address, std::string>> scanForStrings(size_t minSize, const MemoryRegionFilter& filter = {});

	std::vector<std::pair<Address, std::string>> scanForStrings(std::span<const MemoryCopy> memSnap, size_t minSize);

	std::vector<std::pair<Address, std::string>> scanForStrings(const MemoryCopy& memCopy, size_t minSize);

	/**
	 * @brief Scans memory for for wide (UTF-16) strings.
	 *
	 * Identifies contiguous printable character sequences with a minimum length.
	 *
	 * @return A list of (address, wide string) pairs.
	 */
	std::vector<std::pair<Address, std::u16string>> scanForWideStrings(const MemoryRange& memRange, size_t minSize, const MemoryRegionFilter& filter = {});

	std::vector<std::pair<Address, std::u16string>> scanForWideStrings(Address start, Address stop, size_t minSize, const MemoryRegionFilter& filter = {});

	std::vector<std::pair<Address, std::u16string>> scanForWideStrings(size_t minSize, const MemoryRegionFilter& filter = {});

	std::vector<std::pair<Address, std::u16string>> scanForWideStrings(std::span<const MemoryCopy> memSnap, size_t minSize);

	std::vector<std::pair<Address, std::u16string>> scanForWideStrings(const MemoryCopy& memCopy, size_t minSize);

	/**
	 * @brief Searches for a code cave (a contiguous region of unused bytes).
	 *
	 * Typically used for injection or patching purposes. The scan is restricted to 
	 * memory pages that are both readable and executable only. This ensures that 
	 * the returned region is suitable for code execution and unlikely to be modified 
	 * by regular program behavior.
	 *
	 * @return Address of the first suitable code cave, or 0 if none was found.
	 */
	Address scanForCodeCave(const MemoryRange& memRange, size_t minSize, size_t alignment = 1);

	Address scanForCodeCave(Address start, Address stop, size_t minSize, size_t alignment = 1);

	Address scanForCodeCave(size_t minSize, size_t alignment = 1);

	Address scanForCodeCave(std::span<const MemoryCopy> memSnap, size_t minSize, size_t alignment = 1);

	Address scanForCodeCave(const MemoryCopy& memCopy, size_t minSize, size_t alignment = 1);

	/**
	 * @brief Finds all cross-references to a relative address within a module.
	 *
	 * The scan iterates over decoded instructions within the given module and
 	 * collects those whose operands reference the specified relative target address
	 * 
	 * This is commonly used to locate references to functions, globals,
	 * or instruction targets inside executable code.
	 *
	 * @param module                 The module to scan.
	 * @param absoluteTargetAddress  The absolute address being referenced by instructions.
	 *
	 * @return A list of absolute instruction addresses that reference the target.
	 */
	std::vector<Address> findAllCrossRefs(const ProcessImage& module, Address absoluteTargetAddress);

	std::vector<Address> findAllCrossRefs(std::span<const MemoryCopy> memSnap, Address absoluteTargetAddress);

	std::vector<Address> findAllCrossRefs(const MemoryCopy& memCopy, Address absoluteTargetAddress);

	std::vector<Address> findSymbolCrossRefs(const ProcessImage& module, const ImageSymbol& symbol);

	std::vector<Address> findSymbolCrossRefs(const ProcessImage& module, std::span<const MemoryCopy> memSnap, const ImageSymbol& symbol);

	std::vector<Address> findSymbolCrossRefs(const ProcessImage& module, const MemoryCopy& memCopy, const ImageSymbol& symbol);

private:
	CProcess* _backPtr;
	CMemoryModule* _memory;
};


}