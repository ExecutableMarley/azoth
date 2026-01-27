/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <vector>
#include <memory>

#include "../Types/Address.hpp"
#include "PointerEndpoint.hpp"

namespace Azoth
{


class CMemoryModule;

typedef unsigned char BYTE;

/**
 * @brief Cached copy of a contiguous remote memory region.
 * 
 * MemoryCopy provides a mechanism to copy a block of memory from a target
 * process into a local buffer, operate on it locally, and optionally write
 * it back to the target process.
 * 
 * @note No automatic synchronization is performed and the caller is responsible
 * for explicitly calling readIn() and writeBack().
 */
class MemoryCopy
{
    friend class CMemoryModule;
public:
	MemoryCopy();

protected:
	MemoryCopy(CMemoryModule* memory, uint64_t addr, size_t size);

	MemoryCopy(CMemoryModule* memory, PointerEndpoint chainAddr, size_t size);

public:
    /**
     * @brief Check whether this MemoryCopy instance is usable.
     *
     * @return True if a memory module, address, and buffer are properly set.
     */
	bool valid() const;

    //==================================================================
    // Source configuration
    //==================================================================

    /**
     * @brief Set the memory source to an absolute address.
     *
     * @param[in] addr Absolute address in the target process.
     */
	void setAddress(uint64_t addr);

	/**
     * @brief Set the memory source using a pointer-chain endpoint.
     *
     * @param[in] ptrEndpoint Endpoint of a pointer chain.
     */
	void setAddress(PointerEndpoint ptrEndpoint);

    /**
     * @brief Retrieve the resolved base address used for copying.
     *
     * @tparam T Returned integer type (defaults to uint64_t).
     * @return Base address in the target process.
     */
	template<typename T = uint64_t>
	T getBaseAddress() const
	{
		return _chainAddr.get();
	}

    //==================================================================
    // Buffer management
    //==================================================================

    /**
     * @brief Resize the internal buffer.
     *
     * @param[in] newSize New buffer size in bytes.
     *
     * @note This may reallocate the internal buffer. However in this case the content is 
     * carried over
     */
	void resize(size_t newSize);

    //==================================================================
    // Read from target process
    //==================================================================

    /**
     * @brief Read remote memory into the internal buffer.
     *
     * Uses the currently configured memory source.
     *
     * @return True if the read succeeds.
     */
	bool readIn();

    /**
     * @brief Read remote memory from an absolute address.
     *
     * @param[in] addr Base address in the target process.
     * @param[in] size Number of bytes to read (0 = use current size).
     *
     * @return True if the read succeeds.
     */
	bool readIn(uint64_t addr, size_t size = 0);

    //Todo: might be obsolete
    /**
     * @brief Read remote memory from a pointer-chain endpoint.
     *
     * @param[in] ptrEndpoint Resolved endpoint of a pointer chain.
     * @param[in] size        Number of bytes to read (0 = use current size).
     *
     * @return True if the read succeeds.
     */
	bool readIn(PointerEndpoint ptrEndpoint, size_t size = 0);

    //==================================================================
    // Write back to target process
    //==================================================================

    /**
     * @brief Write the internal buffer back to the configured memory source.
     *
     * @return True if the write succeeds.
     */
	bool writeBack();

    /**
     * @brief Write the internal buffer to an absolute address.
     *
     * @param[in] addr Destination address in the target process.
     *
     * @return True if the write succeeds.
     */
	bool writeBack(uint64_t addr);

    /**
     * @brief Write the internal buffer to a pointer-chain endpoint.
     *
     * @param[in] ptrEndpoint Resolved endpoint of a pointer chain.
     *
     * @return True if the write succeeds.
     */
	bool writeBack(PointerEndpoint ptrEndpoint);

    //==================================================================
    // Buffer access
    //==================================================================

    /**
     * @brief Get a pointer to the internal buffer.
     *
     * @return Pointer to the locally cached memory.
     */
	BYTE* getBuffer() const { return this->_buffer.get(); }

    /**
     * @brief Implicit conversion to raw buffer pointer.
     *
     * Intended for convenience when interfacing with low-level APIs.
     */
	operator BYTE* () const { return this->_buffer.get(); }

    /**
     * @brief Get the size of the cached memory block.
     */
	size_t getSize() const { return this->_size; }

    /**
     * @brief Copy data from the internal buffer to an external buffer.
     *
     * @param[in] offset Byte offset within the internal buffer.
     * @param[in] size   Number of bytes to copy.
     * @param[out] buffer Destination buffer.
     *
     * @return True if the operation succeeds.
     */
	bool readFromBuffer(uint64_t offset, size_t size, void* buffer) const;

    /**
     * @brief Copy data from an external buffer into the internal buffer.
     *
     * @param[in] offset Byte offset within the internal buffer.
     * @param[in] size   Number of bytes to copy.
     * @param[in] buffer Source buffer.
     *
     * @return True if the operation succeeds.
     */
	bool writeToBuffer(uint64_t offset, size_t size, void* buffer);

    /**
     * @brief Read a typed value from the internal buffer.
     *
     * @tparam T Value type.
     * @param[in] offset Byte offset within the internal buffer.
     *
     * @return The value, or T{} if out of bounds.
     */
	template <class T> requires std::is_trivially_copyable_v<T>
	T get(uint64_t offset) const
    {
        if (this->_buffer == NULL)
            return T();

        if (offset + sizeof(T) <= this->_size)
            return *reinterpret_cast<T*>(this->_buffer.get() + offset);
        return T();
    }

    /**
     * @brief Write a typed value into the internal buffer.
     *
     * @tparam T Value type.
     * @param[in] offset Byte offset within the internal buffer.
     * @param[in] value  Value to write.
     *
     * @return True if the write succeeds.
     */
	template <class T> requires std::is_trivially_copyable_v<T>
	bool set(uint64_t offset, T value)
    {
        if (this->_buffer == NULL)
            return false;

        if (offset + sizeof(T) <= this->_size)
        {
            *reinterpret_cast<T*>(this->_buffer.get() + offset) = value;
            return true;
        }
        return false;
    }

    //==================================================================
    // Address translation
    //==================================================================

    /**
     * @brief Translate a buffer pointer back to its original remote address.
     *
     * @param[in] ptr Pointer into the internal buffer.
     *
     * @return Corresponding address in the target process, or 0 if invalid.
     */
	uint64_t translate(const void* ptr) const;

    /**
     * @brief Translate multiple buffer pointers back to remote addresses.
     *
     * @param[in] ptrs Pointers into the internal buffer.
     *
     * @return Vector of translated addresses (0 for invalid pointers).
     */
	std::vector<uint64_t> translate(const std::vector<void*>& ptrs) const;

private:
	CMemoryModule*  _memory;
	PointerEndpoint _chainAddr;
	size_t _size;
	std::unique_ptr<BYTE[]> _buffer;
	size_t                  _bufferSize;
};


}