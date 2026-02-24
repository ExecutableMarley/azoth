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


namespace Azoth
{


class CProcess;
class CMemoryModule;


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

	Address findPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	/**
	 * @brief Finds all occurrences of a byte pattern within a memory range.
	 *
	 * @return A list of absolute addresses where the pattern was found.
	 */
	std::vector<Address> findAllPatternEx(const MemoryRange& memRange, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	std::vector<Address> findAllPatternEx(Address start, Address stop, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	std::vector<Address> findAllPatternEx(const Pattern& pattern, const MemoryRegionFilter& filter = {});

	std::vector<Address> findAllPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, const MemoryRegionFilter& filter = {});

	//--------------------------------------------------------
	// Signature-based scanning
	//--------------------------------------------------------


	/**
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
	Address signatureScanEx(const MemoryRange& memRange, const Pattern& pattern, 
		short type, int operatorIndex, int addressOffset);

	Address signatureScanEx(Address start, Address end, const Pattern& pattern,
		short type, int operatorIndex, int addressOffset);

	Address signatureScanEx(const Pattern& pattern,
		short type, int operatorIndex, int addressOffset);

	Address signatureScanEx(const MemoryCopy& memCopy, const Pattern& pattern, 
		short type, int operatorIndex, int addressOffset);

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
	 * @param relativeTargetAddress  Target address relative to the module base.
	 *
	 * @return A list of absolute instruction addresses that reference the target.
	 */
	std::vector<Address> findAllCrossRefs(const ProcessImage& module, Address relativeTargetAddress);

	std::vector<Address> findAllCrossRefs(const MemoryCopy& memCopy, Address relativeTargetAddress);


private:
	CProcess* _backPtr;
	CMemoryModule* _memory;
};


}