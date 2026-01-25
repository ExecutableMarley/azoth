/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "MemoryBlock.hpp"

#include <algorithm>

#include "../ProcessModules/CMemoryModule.hpp"

namespace Azoth
{


MemoryBlock::MemoryBlock()
{
    this->_memory = nullptr;
    this->_chainAddr = PointerEndpoint();
    this->_size = 0;
    this->_ownsMemory = false;
}

MemoryBlock::MemoryBlock(CMemoryModule* mem, Address addr, size_t size, bool ownsMemory)
{
    this->_memory     = mem;
    this->_chainAddr  = PointerEndpoint(addr);
    this->_size       = size;
    this->_ownsMemory = ownsMemory;
}

MemoryBlock::~MemoryBlock()
{
    deallocate();
}

bool MemoryBlock::valid() const noexcept
{
    return _memory && _chainAddr && _size;
}

size_t MemoryBlock::getSize() const noexcept
{
    return _size;
}

bool MemoryBlock::ownsMemory() const noexcept
{
    return _ownsMemory;
}

bool MemoryBlock::contains(uint64_t offset, size_t size) const noexcept
{
    return valid() && offset + size <= _size;
}

bool MemoryBlock::allocate(size_t size, EMemoryProtection protection)
{
    if (!_memory || size == 0)
        return false;

    deallocate();

    auto addr = _memory->virtualAllocate(size, protection);
    if (!addr)
        return false;

    _chainAddr  = PointerEndpoint(addr);
    _size       = size;
    _ownsMemory = true;
    return true;
}

bool MemoryBlock::deallocate()
{
    if (_ownsMemory && _chainAddr.valid())
    {
        if (!_memory->virtualFree(_chainAddr))
            return false;
    }

    _chainAddr  = {};
    _size       = 0;
    _ownsMemory = false;
    return true;
}

bool MemoryBlock::reallocate(size_t newSize, EMemoryProtection protection)
{
    if (!_ownsMemory || !_chainAddr.valid() || newSize == 0)
        return false;

    const size_t copySize = std::min(_size, newSize);

    auto temp = std::make_unique<uint8_t[]>(copySize);
    if (!read(0, copySize, temp.get()))
        return false;

    // Attempt allocation
    auto newAddr = _memory->virtualAllocate(newSize, protection);
    if (!newAddr)
        return false;

    PointerEndpoint oldAddr = _chainAddr;
    size_t oldSize = _size;


    _chainAddr = PointerEndpoint(newAddr);
    _size      = newSize;

    write(0, copySize, temp.get());

    _memory->virtualFree(oldAddr);

    return true;
}

bool MemoryBlock::read(uint64_t offset, size_t size, void* buffer)
{
    if (!contains(offset, size))
        return false;
    return _memory->read(_chainAddr + offset, size, buffer);
}

bool MemoryBlock::write(uint64_t offset, size_t size, void* buffer)
{
    if (!contains(offset, size))
        return false;
    return _memory->write(_chainAddr + offset, size, buffer);
}


}