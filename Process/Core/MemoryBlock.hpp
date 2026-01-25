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

class MemoryBlock
{
    friend class CMemoryModule;
public:
    MemoryBlock();

protected:
    MemoryBlock(CMemoryModule* mem, Address addr, size_t size, bool ownsMemory);

    ~MemoryBlock();
public:

    bool valid() const noexcept;

    size_t getSize() const noexcept;

    bool ownsMemory() const noexcept;

    template<typename T = Address>
	T getBaseAddress() const
	{
		return _chainAddr.get();
	}

    bool contains(uint64_t offset, size_t size = 1) const noexcept;

    bool allocate(size_t size, EMemoryProtection protection = EMemoryProtection::RWX);

    bool deallocate();

    bool reallocate(size_t newSize, EMemoryProtection protection = EMemoryProtection::RWX);

    bool read(uint64_t offset, size_t size, void* buffer);

    template<typename T>
    bool read(uint64_t offset, T& out)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        return read(offset, sizeof(T), &out);
    }

    bool write(uint64_t offset, size_t size, void* buffer);

    template<typename T>
    bool write(uint64_t offset, const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        return write(offset, sizeof(T), &value);
    }

    /**
     * @brief Convert the memory block to its corresponding memory range.
     *
     * The returned range uses half-open interval semantics:
     * [baseAddress, baseAddress + size).
     */
	operator MemoryRange() const { return MemoryRange(_chainAddr, _chainAddr + _size); };

private:
    CMemoryModule*  _memory;
	PointerEndpoint _chainAddr;
	size_t _size;
    bool _ownsMemory = false;
};


}