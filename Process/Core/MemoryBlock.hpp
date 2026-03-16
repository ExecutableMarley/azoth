/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../Types/EMemoryProtection.hpp"
#include "../Types/Address.hpp"
#include "../Types/MemoryRange.hpp"
#include "PointerEndpoint.hpp"

namespace Azoth
{


class CMemoryModule;

/**
 * @brief Helper for viewing and optionally managing remote process memory.
 *
 * MemoryBlock provides direct access to memory in the target process. Unlike
 * `MemoryCopy`, which caches remote bytes locally, this class performs all
 * reads and writes directly in the remote address space, remaining always
 * synchronized with the target.
 * This class's primary value is simpler lifetime and
 * allocation management (allocate/deallocate/reallocate) and convenient
 * offset-based access so callers can work with offsets without manually
 * performing address arithmetic.
 *
 * @note No local caching is performed. Use `MemoryCopy` if a local copy is required.
 */
class MemoryBlock
{
    friend class CMemoryModule;
public:
    /**
     * @brief Constructs an unbound and invalid MemoryBlock
     */
    MemoryBlock();

protected:

    /**
     * @brief Constructs a bound and invalid MemoryBlock
     */
    MemoryBlock(CMemoryModule* mem);

    /**
     * @brief Construct a MemoryBlock bound to an address and memory instance.
     *
     * @param[in] mem        Owning memory module used to perform operations.
     * @param[in] addr       Base address of the block in the target process.
     * @param[in] size       Size of the block in bytes.
     * @param[in] ownsMemory True when this object is responsible for freeing the memory.
     */
    MemoryBlock(CMemoryModule* mem, Address addr, size_t size, bool ownsMemory);

    /**
     * @brief Destructor releases owned memory when appropriate.
     */
    ~MemoryBlock();
public:

    /**
     * @brief Check whether the MemoryBlock is valid and usable.
     *
     * @return True when the block has an associated memory module, an address and a non-zero size.
     */
    bool valid() const noexcept;

    /**
     * @brief Get the size of the memory block.
     *
     * @return Size in bytes.
     */
    size_t getSize() const noexcept;

    /**
     * @brief Query whether this MemoryBlock owns the underlying memory.
     *
     * When true the destructor will attempt to free the remote memory.
     */
    bool ownsMemory() const noexcept;

    /**
     * @brief Retrieve the resolved base address
     *
     * This is the resolved base address in the target process at the time of
     * the call.
     *
     * @tparam T Returned integer type (defaults to Address).
     * @return Base address in the target process.
     */
    template<typename T = Address>
	T getBaseAddress() const
	{
		return _chainAddr.get();
	}

    //==================================================================
    // Queries
    //==================================================================

    /**
     * @brief Check whether the provided range lies within the block.
     *
     * @param[in] offset Byte offset from the block base.
     * @param[in] size   Length in bytes (defaults to 1).
     *
     * @return True when the full range [offset, offset+size) is contained.
     */
    bool contains(uint64_t offset, size_t size = 1) const noexcept;

    //==================================================================
    // Allocation management
    //==================================================================

    /**
     * @brief Allocate a new remote memory region and bind this MemoryBlock to it.
     *
     * @param[in] size       Number of bytes to allocate.
     * @param[in] protection Desired protection flags.
     *
     * @return True on success.
     */
    bool allocate(size_t size, EMemoryProtection protection = EMemoryProtection::RWX);

    /**
     * @brief Free the remote memory if this block owns it.
     *
     * @return True on success or if there was nothing to free.
     */
    bool deallocate();

    /**
     * @brief Reallocate the owned remote memory region.
     * 
     * Allocates a new remote region with the specified size and protection,
     * copies the existing contents to the new region (up to the minimum of
     * old and new size), and releases the previously owned allocation.
     *
     * This operation requires that the MemoryBlock owns its underlying
     * allocation
     * 
     * @param[in] newSize    New size in bytes.
     * @param[in] protection Desired protection flags.
     *
     * @return True if the reallocation succeeded.
     */
    bool reallocate(size_t newSize, EMemoryProtection protection = EMemoryProtection::RWX);

    //==================================================================
    // Read / Write
    //==================================================================

    /**
     * @brief Read bytes from the remote block into a local buffer.
     *
     * @param[in]  offset Byte offset within the block.
     * @param[in]  size   Number of bytes to read.
     * @param[out] buffer Destination buffer.
     *
     * @return True on success.
     */
    bool read(uint64_t offset, size_t size, void* buffer);

    /**
     * @brief Reads a trivially copyable value from the remote block.#
     * 
     * Equivalent to calling read(offset, sizeof(T), &out).
     * 
     * @tparam     T Must be trivially copyable.
     * @param[in]  offset Byte offset relative to the block's base address.
     * @param[out] out    Receives the copied value on success.
     * 
     * @return True if the read operation succeeded.
     */
    template <class T> requires std::is_trivially_copyable_v<T>
    bool read(uint64_t offset, T& out)
    {
        return read(offset, sizeof(T), &out);
    }

    /**
     * @brief Write bytes from a local buffer into the remote block.
     *
     * @param[in] offset Byte offset within the block.
     * @param[in] size   Number of bytes to write.
     * @param[in] buffer Source buffer.
     *
     * @return True on success.
     */
    bool write(uint64_t offset, size_t size, void* buffer);

    /**
     * @brief Writes a trivially copyable value into the remote block.
     *
     * Equivalent to calling write(offset, sizeof(T), &value).
     *
     * @tparam T Must be trivially copyable.
     * @param[in] offset Byte offset relative to the block's base address.
     * @param[in] value  Value to write.
     *
     * @return True if the write operation succeeded.
     */
    template <class T> requires std::is_trivially_copyable_v<T>
    bool write(uint64_t offset, const T& value)
    {
        return write(offset, sizeof(T), &value);
    }

    /**
     * @brief Convert the memory block to a `MemoryRange`.
     *
     * The returned range uses half-open interval semantics: [baseAddress, baseAddress + size).
     */
    operator MemoryRange() const { return MemoryRange(_chainAddr, _chainAddr + _size); };

private:
    CMemoryModule*  _memory;
	PointerEndpoint _chainAddr;
	size_t _size;
    bool _ownsMemory = false;
};


}