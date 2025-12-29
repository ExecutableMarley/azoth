#pragma once

#include "../Types/Address.hpp"
#include "../Types/MemoryRange.hpp"
#include "../Types/Pattern.hpp"

#include "../Core/ProcessImage.hpp"
#include "../Core/MemoryRegion.hpp"
#include "../Core/MemoryCopy.hpp"

class CProcess;
class CMemoryModule;

namespace Azoth
{


class CScannerModule
{
public:
	CScannerModule(CProcess* backPtr);

	//--------------------------------------------------------
	// 
	//--------------------------------------------------------

	uint64_t findPatternEx(uint64_t start, uint64_t stop, const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	uint64_t findPatternEx(const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	uint64_t findPatternEx(const MemoryRange& memRange, const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	uint64_t findPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	std::vector<uint64_t> findAllPatternEx(uint64_t start, uint64_t stop, const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	std::vector<uint64_t> findAllPatternEx(const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	std::vector<uint64_t> findAllPatternEx(const MemoryRange& memRange, const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	std::vector<uint64_t> findAllPatternEx(const MemoryCopy& memCopy, const Pattern& pattern, ProtectionFilter protectionFilter = ProtectionFilter());

	//--------------------------------------------------------
	// 
	//--------------------------------------------------------

	uint64_t signatureScanEx(uint64_t start, uint64_t end, const Pattern& pattern,
		short type, int operatorIndex, int addressOffset);

	uint64_t signatureScanEx(const Pattern& pattern,
		short type, int operatorIndex, int addressOffset);

	uint64_t signatureScanEx(const MemoryRange& memRange, const Pattern& pattern, 
		short type, int operatorIndex, int addressOffset);

	uint64_t signatureScanEx(const MemoryCopy& memCopy, const Pattern& pattern, 
		short type, int operatorIndex, int addressOffset);

	//--------------------------------------------------------
	// 
	//--------------------------------------------------------

	uint64_t findNextValue(uint64_t start, uint64_t stop, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter = ProtectionFilter());

	uint64_t findNextValue(MemoryRange& memRange, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter = ProtectionFilter());

	uint64_t findNextValue(MemoryCopy& memCopy, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter = ProtectionFilter());

	template <typename T>
	uint64_t findNextValue(uint64_t start, uint64_t stop, T value, ProtectionFilter protectionFilter = ProtectionFilter())
	{
		return findNextValue(start, stop, &value, sizeof(value), protectionFilter);
	}

	template <typename T>
	uint64_t findNextValue(MemoryRange& memRange, T value, ProtectionFilter protectionFilter = ProtectionFilter())
	{
		return findNextValue(memRange.startAddr, memRange.stopAddr, &value, sizeof(value), protectionFilter);
	}

	template <typename T>
	uint64_t findNextValue(MemoryCopy& memCopy, T value, ProtectionFilter protectionFilter = ProtectionFilter())
	{
		return findNextValue(memCopy, &value, sizeof(value), protectionFilter);
	}

	std::vector<uint64_t> findAllValues(uint64_t start, uint64_t stop, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter = ProtectionFilter());

	std::vector<uint64_t> findAllValues(MemoryRange& memRange, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter = ProtectionFilter());

	std::vector<uint64_t> findAllValues(MemoryCopy& memCopy, BYTE* value, size_t valueSize, ProtectionFilter protectionFilter = ProtectionFilter());

	template <typename T>
	std::vector<uint64_t> findAllValues(uint64_t start, uint64_t stop, T value, ProtectionFilter protectionFilter = ProtectionFilter())
	{
		return findAllValues(start, stop, &value, sizeof(value), protectionFilter);
	}

	template <typename T>
	std::vector<uint64_t> findAllValues(MemoryRange& memRange, T value, ProtectionFilter protectionFilter = ProtectionFilter())
	{
		return findAllValues(memRange.startAddr, memRange.stopAddr, &value, sizeof(value), protectionFilter);
	}

	template <typename T>
	std::vector<uint64_t> findAllValues(MemoryCopy& memCopy, T value, ProtectionFilter protectionFilter = ProtectionFilter())
	{
		return findAllValues(memCopy, &value, sizeof(value), protectionFilter);
	}

	//--------------------------------------------------------
	// 
	//--------------------------------------------------------

	std::vector<std::pair<uint64_t, std::string>> scanForStrings(uint64_t start, uint64_t stop, size_t minSize);

	std::vector<std::pair<uint64_t, std::wstring>> scanForWideStrings(uint64_t start, uint64_t stop, size_t minSize);

	uint64_t scanForCodeCave(uint64_t start, uint64_t stop, size_t minSize);

	uint64_t scanForCodeCave(size_t minSize);

	uint64_t scanForCodeCave(const MemoryRange& memRange, size_t minSize);

	uint64_t scanForCodeCave(const MemoryCopy& memCopy, size_t minSize);

	std::vector<uint64_t> findAllCrossRefs(const ProcessImage& module, uint64_t relativeTargetAddress);

	std::vector<uint64_t> findAllCrossRefs(const MemoryCopy& memCopy, uint64_t relativeTargetAddress);



private:
	CProcess* _backPtr;
	CMemoryModule* _memory;
};


}