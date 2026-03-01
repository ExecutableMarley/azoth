/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <string>
#include <vector>
#include <list>
#include <map>
#include <tuple>
#include <array>
#include <memory>
#include <cassert>

#include "../Types/Address.hpp"
#include "../Core/MemoryRegion.hpp"
#include "../Platform/IPlatformLink.hpp"


namespace Azoth
{

typedef uint8_t BYTE;

class CProcess;


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
          assert(_backPtr && platformLink);
	}

     CMemoryModule(const CMemoryModule&) = delete;

     CMemoryModule& operator=(const CMemoryModule&) = delete;
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
	bool read(Address addr, size_t size, void* buffer)
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
	template <class T> requires std::is_trivially_copyable_v<T>
	bool read(Address addr, T& buffer)
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
	bool readString(Address addr, std::string& buffer, size_t maxSize = 256)
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
	template <class T> requires std::is_trivially_copyable_v<T>
	T read(Address addr, const T& defaultValue = T())
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
	std::string readString(Address addr, size_t maxSize = 256)
	{
		std::vector<char> buffer(maxSize + 1, 0);
		if (!this->read(addr, maxSize, buffer.data()))
			return {};
		buffer[maxSize] = 0;
		return std::string(buffer.data());
	}

	std::string readString(Address addr, std::vector<uint64_t> offsets, size_t size = 256)
	{
		for (uint64_t offset : offsets)
		{
			this->read(addr, sizeof(Address), &addr);
			addr += offset;
		}
		return this->readString(addr, size);
	}

     /**
      * @brief Read a pointer-sized value from the target process.
      *
      * Reads a pointer value whose size matches the architecture of the target
      * process (e.g. 4 bytes on 32-bit targets, 8 bytes on 64-bit targets) and
      * zero-extends it into a 64-bit output value.
      *
      * @param addr Address from which the pointer value is read.
      * @param out  Receives the pointer value.
      *
      * @return True if the pointer was successfully read; false otherwise.
      */
     bool readPtr(Address addr, Address& out);

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
	bool write(Address addr, size_t size, const void* buffer)
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
	template <class T> requires std::is_trivially_copyable_v<T>
	bool write(Address addr, const T& buffer)
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
	bool write(Address addr, const std::string& buffer)
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
	bool write(Address addr, const std::wstring& buffer)
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
	bool queryMemory(Address addr, MemoryRegion& memoryRegion)
	{
		return _platformLink->queryMemory(addr, memoryRegion);
	}

     /**
     * @brief Query all memory regions within an address range.
     *
     * Enumerates memory regions in the interval [startAddr, maxAddr) and
     * returns those that are in the committed state and satisfy the
     * provided filter predicate.
     *
     * The filter is optional. If no filter is provided, all committed
     * regions within the specified range are returned.
     *
     * @param startAddr Inclusive start address of the query range.
     * @param maxAddr   Exclusive end address of the query range.
     * @param filter    Optional callable of type
     *                  std::function<bool(const MemoryRegion&)>
     *                  used to decide whether a region should be included.
     *
     * @return A vector containing all matching memory regions.
     */
     std::vector<MemoryRegion> queryAllMemoryRegions(Address startAddr, Address maxAddr, const MemoryRegionFilter& filter = {});

     /**
     * @brief Query all memory regions within an address range.
     *
     * Enumerates memory regions in the interval [startAddr, maxAddr) and
     * returns those that are in the committed state and satisfy the
     * provided filter predicate.
     *
     * The filter is optional. If no filter is provided, all committed
     * regions within the specified range are returned.
     *
     * @param memRange  The query Range
     * @param filter    Optional callable of type
     *                  std::function<bool(const MemoryRegion&)>
     *                  used to decide whether a region should be included.
     *
     * @return A vector containing all matching memory regions.
     */
     std::vector<MemoryRegion> queryAllMemoryRegions(const MemoryRange& memRange, const MemoryRegionFilter& filter = {})
     {
          return queryAllMemoryRegions(memRange.startAddr, memRange.stopAddr, filter);
     }

private:

     //Placeholder
     bool setError(EPlatformError errorCode)
     {
          return _platformLink->setError(errorCode);
     }

     //Not needed with current design
     bool addProtectExRestore(Address protectAddr, size_t size, EMemoryProtection oldProtect)
     {
          auto it = _pageProtectRestoreMap.find(protectAddr);
          if (it == _pageProtectRestoreMap.end())
          {
               _pageProtectRestoreMap[protectAddr] = ProtectRestorePoint(size, oldProtect);
               return true;
          }
          return false;
     }
public:

     /**
     * @brief Change the protection of a memory range.
     *
     * This function enforces a **no-overlap rule** for tracked protection changes:
     * - A protection change may only be applied to a range that does **not overlap**
     *   with any other tracked protection range.
     * - The only exception is an **exact match** with an existing tracked range
     *   (same base address and size).
     * 
     * @param addr            Base address of the range.
     * @param size            Size of the range in bytes.
     * @param newProtect      New protection flags.
     * @param pOldProtection  Optional output for the previous protection.
     * 
     * @return True if the protection change succeeded. On failure, an appropriate
     *         EPlatformError is set.
     */
	bool virtualProtect(Address addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* pOldProtection = nullptr);

     /**
      * @brief Restore the original protection of a tracked memory range.
      *
      * Looks up a previously tracked protection change by its base address and
      * restores the original protection recorded at the time of the change.
      *
      * If no tracked entry exists for the given address, the function succeeds
      * without performing any operation.
      *
      * @param addr Base address of the tracked protection range.
      *
      * @return True if the protection was successfully restored or if no tracked
      *         entry exists, false if the platform call fails.
      */
     bool restoreProtection(Address addr);
private:

     bool addAllocationRestoreEntry(Address allocatedAddr, size_t allocationSize)
     {
          if (_allocRestoreMap.count(allocatedAddr) == 0)
          {
               _allocRestoreMap[allocatedAddr] = AllocRestorePoint(allocationSize);
               return true;
          }
          //This should not happen normally?
          return false;
     }
public:

    /**
     * @brief Allocate virtual memory in the process.
     *
     * @param addr        Preferred base address, or 0 for automatic selection.
     * @param size        Size of the allocation in bytes.
     * @param protection  Initial protection flags.
     *
     * @return Base address of the allocated memory, or 0 on failure.
     */
	Address virtualAllocate(Address addr, size_t size, EMemoryProtection protection = EMemoryProtection::RWX)
	{
          Address allocatedAddr = _platformLink->virtualAllocate(addr, size, protection);
          if (allocatedAddr)
          {
               addAllocationRestoreEntry(allocatedAddr, size);
          }

          return allocatedAddr;
	}

     /**
     * @brief Allocate virtual memory in the process.
     *
     * @param size        Size of the allocation in bytes.
     * @param protection  Initial protection flags.
     *
     * @return Base address of the allocated memory, or 0 on failure.
     */
     Address virtualAllocate(size_t size, EMemoryProtection protection = EMemoryProtection::RWX)
	{
          return virtualAllocate(0, size, protection);
	}

     /**
      * @brief Free a virtual memory allocation.
      *
      * By default, this function only frees memory that is **tracked** by this
      * memory module. If the allocation is tracked, it is released via the platform
      * backend and removed from the internal tracking map.
      *
      * If `allowUntracked` is set to true, the function will attempt to free the
      * memory even if no tracked allocation exists for the given address.
      *
      * @param addr           Base address of the allocation to free.
      * @param allowUntracked If true, allows freeing memory that is not tracked
      *                       by this module.
      *
      * @return True if the memory was successfully freed; false if the address is
      *         untracked and `allowUntracked` is false, or if the platform call fails.
      *         On failure, an appropriate EPlatformError is set.
      */
	bool virtualFree(Address addr, bool allowUntracked = false);

     /**
      * @brief Restore (free) a tracked virtual memory allocation.
      *
      * This is a strict variant of @ref virtualFree that only succeeds if the allocation
      * is currently tracked by this memory module.
      * 
      * @param addr Base address of the tracked allocation.
      */
     bool restoreAllocation(Address addr)
     {
          return virtualFree(addr, false);
     }

private:

     bool addMemoryRestorePoint(Address addr, size_t size);
public:

     bool patchReadOnlyMemory(Address addr, size_t size, const BYTE* buffer);

     bool restoreReadOnlyMemory(Address addr);

private:
    struct CodeRestorePoint
    {
        size_t size;
        std::unique_ptr<uint8_t[]> originalBytes;
    };

    struct ProtectRestorePoint
    {
        size_t size;
        EMemoryProtection origProtect;
    };

    struct AllocRestorePoint
    {
        size_t size;
    };

	CProcess* _backPtr;
	IPlatformLink* _platformLink;

     std::map<Address, CodeRestorePoint>    _codeMemoryRestoreMap;
     std::map<Address, ProtectRestorePoint> _pageProtectRestoreMap;
     std::map<Address, AllocRestorePoint>   _allocRestoreMap;
};


}