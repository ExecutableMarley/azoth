#pragma once

#include <stdint.h>
#include <vector>
#include <memory>

#include "PointerEndpoint.hpp"

namespace Azoth
{


class CMemoryModule;

typedef unsigned char BYTE;

//Todo: Adjust comments (Currently copy pasted from old version)

/**
 * @brief Cached copy of a contiguous remote memory region.
 */
class MemoryCopy
{
public:
	MemoryCopy();
	MemoryCopy(CMemoryModule* memory, uint64_t addr, size_t size);
	MemoryCopy(CMemoryModule* memory, PointerEndpoint chainAddr, size_t size);

	bool valid() const;

	/**
	* @brief Sets the memory source to an absolute address.
	*
	* @param[in] addr The absolute memory address to copy from.
	*/
	void setAddress(uint64_t addr);

	/**
	* @brief Sets the memory source using a pointer chain.
	*
	* @param[in] chainAddr The resolved endpoint of a pointer chain.
	*/
	void setAddress(PointerEndpoint ptrEndpoint);

	/**
		* @brief Retrieves the address value stored in the `_chainAddr` member variable.
		*
		* @tparam T The type of the pointer value to be returned. Defaults to `uint64_t`.
		* @return The pointer value stored in `_chainAddr`.
		*/
	template<typename T = uint64_t>
	uint64_t getBaseAddress() const
	{
		return _chainAddr.get();
	}

	/**
		* @brief Resizes the internal buffer for the memory copy operation.
		*
		* @param[in] newSize The new size of the buffer in bytes.
		*
		* @note This method may reallocate the internal buffer.
		*/
	void resize(size_t newSize);

	/**
		* @brief Reads data into the internal buffer using the currently set memory source.
		*
		* @return `true` if the read operation succeeds, `false` otherwise.
		*
		* @note The memory source must be set before calling this method.
		*/
	bool readIn();

	/**
		* @brief Reads data into the internal buffer from an absolute address.
		*
		* @param[in] addr The absolute memory address to read from.
		* @param[in] size The number of bytes to read. If `0`, the currently set size is used.
		* @return `true` if the read operation succeeds, `false` otherwise.
		*/
	bool readIn(uint64_t addr, size_t size = 0);

	/**
		* @brief Reads data into the internal buffer from a pointer chain.
		*
		* @param[in] chainAddr The resolved endpoint of a pointer chain.
		* @param[in] size The number of bytes to read. If `0`, the currently set size is used.
		* @return `true` if the read operation succeeds, `false` otherwise.
		*/
	bool readIn(PointerEndpoint ptrEndpoint, size_t size = 0);

	/**
		* @brief Writes the internal buffer back to the currently set memory address.
		*
		* @return `true` if the write operation succeeds, `false` otherwise.
		*
		* @note The memory source must be set before calling this method.
		*/
	bool writeBack();

	/**
		* @brief Writes the internal buffer to a specified absolute address.
		*
		* @param[in] addr The absolute memory address to write to.
		* @return `true` if the write operation succeeds, `false` otherwise.
		*/
	bool writeBack(uint64_t addr);

	/**
		* @brief Writes the internal buffer to a specified pointer chain endpoint.
		*
		* @param[in] chainAddr The resolved endpoint of a pointer chain.
		* @return `true` if the write operation succeeds, `false` otherwise.
		*/
	bool writeBack(PointerEndpoint ptrEndpoint);

	/**
		* @brief Retrieves a pointer to the internal buffer.
		*
		* @return A pointer to the internal buffer containing the copied memory.
		*/
	BYTE* getBuffer() const { return this->_buffer.get(); }

	/**
		* @brief Implicit conversion operator to a BYTE pointer.
		*
		* @return A pointer to the internal buffer.
		*/
	operator BYTE* () const { return this->_buffer.get(); }

	/**
		* @brief Retrieves the size of the memory block being copied.
		*
		* @return The size of the memory block in bytes.
		*/
	size_t getSize() const { return this->_size; }

	/**
		* @brief Reads a portion of the internal buffer into an external buffer.
		*
		* @param[in] offset The starting offset within the internal buffer.
		* @param[in] size The number of bytes to copy.
		* @param[out] buffer A pointer to the destination buffer where the data will be copied.
		* @return `true` if the read operation succeeds, `false` otherwise.
		*
		* @note This function fails if `offset + size` exceeds the internal buffer size.
		*/
	bool readFromBuffer(uint64_t offset, size_t size, void* buffer) const;

	/**
	* @brief Writes data from an external buffer into the internal buffer.
	*
	* @param[in] offset The starting offset within the internal buffer.
	* @param[in] size The number of bytes to copy.
	* @param[in] buffer A pointer to the source buffer containing the data to write.
	* @return `true` if the write operation succeeds, `false` otherwise.
	*
	* @note This function fails if `offset + size` exceeds the internal buffer size.
	*/
	bool writeToBuffer(uint64_t offset, size_t size, void* buffer);

	/**
	* @brief Retrieves a value of type `val` from the internal buffer at the given offset.
	*
	* @tparam val The type of value to read.
	* @param[in] offset The offset within the internal buffer.
	* @return The retrieved value, or `val()` if `offset + sizeof(val)` exceeds the buffer size.
	*/
	template <class val>
	val get(uint64_t offset) const;

	/**
	* @brief Writes a value of type `val` into the internal buffer at the given offset.
	*
	* @tparam val The type of value to write.
	* @param[in] offset The offset within the internal buffer.
	* @param[in] value The value to write.
	* @return `true` if the write operation succeeds, `false` otherwise.
	*
	* @note This function fails if `offset + sizeof(val)` exceeds the buffer size.
	*/
	template <class val>
	bool set(uint64_t offset, val value);

	/**
	* @brief Translates a pointer to the internal buffer into the original memory address.
	*
	* @param[in] ptr A pointer to a location within the internal buffer.
	* @return The original memory address corresponding to `ptr`, or `0` if `ptr` is outside the buffer range.
	*/
	uint64_t translate(const void* ptr) const;

	/**
	* @brief Translates multiple pointers to the internal buffer into their original memory addresses.
	*
	* @param[in] ptrs A vector of pointers, each pointing to a location within the internal buffer.
	* @return A vector of original memory addresses corresponding to each pointer. Any out-of-bounds pointers are translated to `0`.
	*/
	std::vector<uint64_t> translate(std::vector<void*> ptrs) const;

private:
	CMemoryModule*  _memory;
	PointerEndpoint _chainAddr;
	size_t _size;
	std::unique_ptr<BYTE[]> _buffer;
	size_t                  _bufferSize;
};


}