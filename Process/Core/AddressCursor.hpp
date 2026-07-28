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
 * @brief Represents a pattern scan match with utilities for address resolution.
 */
class AddressCursor
{
	/*
	Experimental class to easily allow the modification of pattern scan results.
	*/
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

	bool insideBuffer(Address addr, size_t size) const
	{
		if (!_buffer || _invalid) return false;
		return (addr >= _bufferStart) && ((addr + size) <= (_bufferStart + _bufferSize));
	}

	bool readBytes(Address addr, void* outBuf, size_t size) const;

public:
	AddressCursor() : _invalid(true) {}

    AddressCursor(Address addr, const CMemoryModule* mem = nullptr)
        : _address(addr), _invalid(addr == 0), _memory(mem) {}

    AddressCursor(Address addr, std::shared_ptr<const BYTE[]> buf, size_t bufSize, Address bufStart, const CMemoryModule* mem = nullptr)
        : _address(addr), _invalid(addr == 0), _buffer(std::move(buf)), _bufferSize(bufSize), _bufferStart(bufStart), _memory(mem) {}

    static AddressCursor Invalid() {
        return AddressCursor(0, nullptr);
    }

    Address get() const { return _address; }

    AddressCursor add(uint64_t offset) const {
		if (_invalid) return Invalid();
        AddressCursor next = *this;
        next._address += offset;
        return next;
    }
    
    AddressCursor sub(uint64_t offset) const {
		return add(-offset);
    }

    AddressCursor read4(uint64_t offset = 0) const {
        uint32_t val = 0;
        if (!readBytes(_address + offset, &val, sizeof(val))) return Invalid();
        
        AddressCursor next = *this;
        next._address = static_cast<Address>(val);
        return next;
    }

    AddressCursor read8(uint64_t offset = 0) const {
        uint64_t val = 0;
        if (!readBytes(_address + offset, &val, sizeof(val))) return Invalid();

        AddressCursor next = *this;
        next._address = static_cast<Address>(val);
        return next;
    }

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