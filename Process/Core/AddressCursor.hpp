/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../Types/Address.hpp"

#include <cstdint>
#include <memory>

namespace Azoth
{


class CMemoryModule;

typedef unsigned char BYTE;

/**
 * @brief A lightweight, chainable cursor for navigating and resolving memory addresses.
 */
class AddressCursor
{
public:
    friend class CScannerModule;

	static constexpr size_t LOCAL_BUFFER_SIZE = 128;

private:
    Address _address;
	bool _invalid;

	std::shared_ptr<const BYTE[]> _buffer;
	Address _bufferStart = 0;
    size_t _bufferSize = 0;

	const CMemoryModule* _memory = nullptr;

    /**
     * @brief Checks if a contiguous block of memory resides completely within the local cache window.
     */
	bool insideBuffer(Address addr, size_t size) const
	{
		if (!_buffer || _invalid) return false;
		return (addr >= _bufferStart) && ((addr + size) <= (_bufferStart + _bufferSize));
	}

    /**
     * @brief Reads memory from the local cache window first, falling back to dynamic remote read.
     */
	bool readBytes(Address addr, void* outBuf, size_t size) const;

public:
    /**
     * @brief Constructs an uninitialized, invalid cursor.
     */
	AddressCursor() : _invalid(true) {}

    /**
     * @brief Constructs a cursor pointed at a raw target address.
     * @param addr The absolute target address.
     * @param mem Pointer to the memory reader interface for dynamic read fallbacks.
     */
    AddressCursor(Address addr, const CMemoryModule* mem = nullptr)
        : _address(addr), _invalid(addr == 0), _memory(mem) {}

        /**
     * @brief Constructs a cursor with an associated local memory cache.
     * @param addr Target address in the remote address space.
     * @param buf Shared pointer holding the locally cached byte window.
     * @param bufSize Size of the cached memory block in bytes.
     * @param bufStart The remote base address corresponding to the start of buf.
     * @param mem Pointer to the memory reader interface for dynamic read fallbacks.
     */
    AddressCursor(Address addr, std::shared_ptr<const BYTE[]> buf, size_t bufSize, Address bufStart, const CMemoryModule* mem = nullptr)
        : _address(addr), _invalid(addr == 0), _buffer(std::move(buf)), _bufferSize(bufSize), _bufferStart(bufStart), _memory(mem) {}

    /**
     * @brief Creates an explicit sentinel invalid cursor state.
     */
    static AddressCursor Invalid() {
        return AddressCursor(0, nullptr);
    }

    /// Checks if the cursor is in a valid state.
    bool isValid() const { return !_invalid; }

    /// Gets the raw address represented by the cursor.
    Address get() const { return _address; }

    /// Implicit conversion to Address for direct arithmetic or variable assignment.
    operator Address() const { return _address; }

    /**
     * @brief Offsets the cursor forward by a given byte count.
     * @param offset Byte count to add to the current address.
     * @return A new AddressCursor moved forward, or Invalid if already invalid.
     */
    AddressCursor add(uint64_t offset) const {
		if (_invalid) return Invalid();
        AddressCursor next = *this;
        next._address += offset;
        return next;
    }
    
    /**
     * @brief Offsets the cursor backward by a given byte count.
     * @param offset Byte count to subtract from the current address.
     * @return A new AddressCursor moved backward, or Invalid if already invalid.
     */
    AddressCursor sub(uint64_t offset) const {
		return add(-offset);
    }

    /**
     * @brief Dereferences a 32-bit unsigned integer (pointer/value) at the target offset.
     * @param offset Offset from current address where the read occurs.
     * @return A new AddressCursor pointed to the dereferenced 32-bit target, or Invalid on failure.
     */
    AddressCursor read4(uint64_t offset = 0) const {
        uint32_t val = 0;
        if (!readBytes(_address + offset, &val, sizeof(val))) return Invalid();
        
        AddressCursor next = *this;
        next._address = static_cast<Address>(val);
        return next;
    }

    /**
     * @brief Dereferences a 64-bit unsigned integer (pointer/value) at the target offset.
     * @param offset Offset from current address where the read occurs.
     * @return A new AddressCursor pointed to the dereferenced 64-bit target, or Invalid on failure.
     */
    AddressCursor read8(uint64_t offset = 0) const {
        uint64_t val = 0;
        if (!readBytes(_address + offset, &val, sizeof(val))) return Invalid();

        AddressCursor next = *this;
        next._address = static_cast<Address>(val);
        return next;
    }

    /**
     * @brief Resolves x86/x64 signed RIP-relative displacement targets.
     * 
     * Computes target address as: `_address + instructionSize + (int32_t)displacement`.
     *
     * @param offset Offset from current address to the start of the 4-byte displacement operand.
     * @param instructionSize Total length of the instruction containing the displacement. 
     *                        Defaults to `offset + 4` (standard for trailing displacements).
     * @return A new AddressCursor pointing to the absolute resolved target, or Invalid on failure.
     */
    AddressCursor readRel(uint64_t offset = 0, size_t instructionSize = 0) const {
        int32_t relAddr = 0;
        Address dispAddr = _address + offset;
        
        if (!readBytes(dispAddr, &relAddr, sizeof(relAddr))) return Invalid();

        if (instructionSize == 0) {
            instructionSize = offset + sizeof(int32_t); // Standard RIP-relative calculation
        }

        Address resolvedAddr = _address + instructionSize + relAddr;
        AddressCursor next = *this;
        next._address = resolvedAddr;
        return next;
    }
};


} // namespace Azoth