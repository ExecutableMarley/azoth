#pragma once

#include <string>
#include <vector>
#include <list>
#include <map>
#include <tuple>
#include <array>
#include <memory>

#include "../Types/Address.hpp"
#include "../Core/MemoryRegion.hpp"
#include "../Platform/IPlatformLink.hpp"

class CProcess;

namespace Azoth
{


/**
 * @brief Platform-independent interface for remote process memory manipulation.
 *
 * CMemoryModule provides a unified abstraction for reading, writing,
 * querying, and managing virtual memory of a target process.
 *
 * All addresses passed to this interface are interpreted as addresses
 * in the virtual address space of a *remote* process.
 */
class CMemoryModule
{
public:
	CMemoryModule(CProcess* backPtr, IPlatformLink* platformLink) :
		_backPtr(backPtr), _platformLink(platformLink)
	{

	}
public:

	//=== Read Memory ===//

    /**
     * @brief Read raw bytes from a memory address.
     *
     * @param addr   Source address to read from.
     * @param size   Number of bytes to read.
     * @param buffer Destination buffer.
     *
     * @return True if the read succeeded.
     */
	bool read(uint64_t addr, size_t size, void* buffer)
	{
		return _platformLink->read(addr, size, buffer);
	}

	/**
     * @brief Read a trivially-copyable object from memory.
     *
     * @tparam T     Type to read.
     * @param addr  Source address.
     * @param buffer Destination object.
     *
     * @return True if the read succeeded.
     */
	template<class T>
	bool read(uint64_t addr, T& buffer)
	{
		return this->read(addr, sizeof(buffer), &buffer);
	}

	/**
     * @brief Read a null-terminated string from memory.
     *
     * Reads up to @p maxSize bytes.
     *
     * @param addr    Source address.
     * @param buffer  Destination string.
     * @param maxSize Maximum number of bytes to read.
     *
     * @return True if the read succeeded.
     */
	bool readString(uint64_t addr, std::string& buffer, size_t maxSize = 256)
	{
		std::vector<char> v(maxSize + 1, 0);
		if (!this->read(addr, maxSize, v.data()))
			return false;
		v[maxSize] = 0;
		buffer = std::string(v.data());
		return true;
	}

    /**
     * @brief Read a value from memory or return a default on failure.
     *
     * @tparam T            Type to read.
     * @param addr          Source address.
     * @param defaultValue  Value returned if the read fails.
     *
     * @return Read value or @p defaultValue on failure.
     */
	template <class T>
	T read(uint64_t addr, const T& defaultValue = T())
	{
		T res;
		if (!this->read(addr, sizeof(res), &res))
		{
			return defaultValue;
		}
		return res;
	}

	/**
     * @brief Read a null-terminated string from memory.
     *
     * @param addr    Source address.
     * @param maxSize Maximum number of bytes to read.
     *
     * @return The read string, or an empty string on failure.
     */
	std::string readString(uint64_t addr, size_t maxSize = 256)
	{
		std::vector<char> buffer(maxSize + 1, 0);
		if (!this->read(addr, maxSize, buffer.data()))
			return {};
		buffer[maxSize] = 0;
		return std::string(buffer.data());
	}

	//=== Write Memory ===//

    /**
     * @brief Write raw bytes to a memory address.
     *
     * @param addr   Destination address.
     * @param size   Number of bytes to write.
     * @param buffer Source buffer.
     *
     * @return True if the write succeeded.
     */
	bool write(uint64_t addr, size_t size, const void* buffer)
	{
		return _platformLink->write(addr, size, buffer);
	}

    /**
     * @brief Write a trivially-copyable object to memory.
     *
     * @tparam T    Type to write.
     * @param addr Destination address.
     * @param buffer Source object.
     *
     * @return True if the write succeeded.
     */
	template <class T>
	bool write(uint64_t addr, T& buffer)
	{
		return write(addr, sizeof(buffer), &buffer);
	}

    /**
     * @brief Write a null-terminated UTF-8 string to memory.
     *
     * @param addr   Destination address.
     * @param buffer String to write.
     *
     * @return True if the write succeeded.
     */
	bool write(uint64_t addr, const std::string& buffer)
	{
		return write(addr, buffer.size() + 1, (void*)buffer.c_str());
	} //Todo: Enforce Max size?

    /**
     * @brief Write a null-terminated wide string to memory.
     *
     * @param addr   Destination address.
     * @param buffer Wide string to write.
     *
     * @return True if the write succeeded.
     */
	bool write(uint64_t addr, const std::wstring& buffer)
	{
		return write(addr, (buffer.size() + 1) * sizeof(wchar_t), (void*)buffer.c_str());
	} //Todo: Enforce Max size?

	//=== Query Memory ===//

    /**
     * @brief Query information about a memory region containing an address.
     *
     * @param addr         Address to query.
     * @param memoryRegion Output structure describing the region.
     *
     * @return True if the query succeeded.
     */
	bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion)
	{
		return _platformLink->queryMemory(addr, memoryRegion);
	}

	/**
     * @brief Query all memory regions in an address range.
     *
     * Regions can be filtered using a ProtectionFilter to match specific
     * access requirements.
     *
     * @param startAddr        Inclusive start address of the query range.
     * @param maxAddr          Exclusive end address of the query range.
     * @param protectionFilter Protection filter to apply.
     *
     * @return List of matching memory regions.
     */
	std::vector<MemoryRegion> queryAllMemoryRegions(uint64_t startAddr, uint64_t maxAddr, ProtectionFilter protectionFilter)
	{
		std::vector<MemoryRegion> regionList;
		for (uint64_t i = startAddr; i < maxAddr;)
		{
			MemoryRegion mr;
			if (queryMemory(i, mr))
			{
				if (mr.state == EMemoryState::Committed && protectionFilter.matchesProtection(mr.protection))
				{
					regionList.push_back(mr);
				}
				i = mr.baseAddress + mr.regionSize;;
			}
			else
			{
				break;
			}
		}

		return regionList;
	}

    /**
     * @brief Change the protection of a memory range.
     *
     * @param addr        Base address of the range.
     * @param size        Size of the range in bytes.
     * @param newProtect  New protection flags.
     * @param oldProtect  Optional output for the previous protection.
     *
     * @return True if the protection change succeeded.
     */
	bool virtualProtect(uint64_t addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* oldProtect)
	{
		return _platformLink->virtualProtect(addr, size, newProtect, oldProtect);
	}

    /**
     * @brief Allocate virtual memory in the process.
     *
     * @param addr        Preferred base address, or 0 for automatic selection.
     * @param size        Size of the allocation in bytes.
     * @param protection  Initial protection flags.
     *
     * @return Base address of the allocated memory, or 0 on failure.
     */
	uint64_t virtualAllocate(uint64_t addr, size_t size, EMemoryProtection protection = EMemoryProtection::RWX)
	{
		return _platformLink->virtualAllocate(addr, size, protection);
	}

    /**
     * @brief Free previously allocated virtual memory.
     *
     * @param addr Base address of the allocation.
     *
     * @return True if the memory was successfully released.
     */
	bool virtualFree(uint64_t addr)
	{
		return _platformLink->virtualFree(addr);
	}

private:
	CProcess* _backPtr;
	IPlatformLink* _platformLink;
};


}