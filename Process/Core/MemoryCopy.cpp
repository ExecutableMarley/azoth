/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "MemoryCopy.hpp"

#include <algorithm>
#include <cstring>

#include "../ProcessModules/CMemoryModule.hpp"

#undef min
#undef max

namespace Azoth
{


MemoryCopy::MemoryCopy()
{
    this->_memory     = NULL;
    this->_chainAddr  = PointerEndpoint();
    this->_size       = 0;
    this->_buffer     = NULL;
    this->_bufferSize = 0;
}

MemoryCopy::MemoryCopy(CMemoryModule* mem, uint64_t addr, size_t size)
{
    this->_memory     = mem;
    this->_chainAddr  = PointerEndpoint(addr);
    this->_size       = size;
    this->_buffer     = std::make_unique<BYTE[]>(size);
    this->_bufferSize = size;
    readIn();
}

MemoryCopy::MemoryCopy(CMemoryModule* mem, PointerEndpoint chainAddr, size_t size)
{
    this->_memory     = mem;
    this->_chainAddr  = chainAddr;
    this->_size       = size;
    this->_buffer     = std::make_unique<BYTE[]>(size);
    this->_bufferSize = size;
    readIn();
}

bool MemoryCopy::valid() const
{
    return this->_memory != NULL && this->_chainAddr.get() != Address::null() && this->_size > 0 && this->_buffer != NULL;
}

void MemoryCopy::setAddress(uint64_t addr)
{
    this->_chainAddr = PointerEndpoint(addr);
}

void MemoryCopy::setAddress(PointerEndpoint chainAddr)
{
    this->_chainAddr = chainAddr;
}

void MemoryCopy::resize(size_t newSize)
{
    if (newSize != this->_size)
    {
        std::unique_ptr<BYTE[]> newBuffer = std::make_unique<BYTE[]>(newSize);
        if (this->_buffer)
        {
            std::memcpy(newBuffer.get(), this->_buffer.get(), std::min(this->_size, newSize));
        }
        this->_buffer = std::move(newBuffer);
        this->_size = newSize;
        this->_bufferSize = newSize;
    }
}

bool MemoryCopy::readIn()
{
    if (this->_chainAddr.valid())
        return this->_memory->read(this->_chainAddr, this->_size, this->_buffer.get());
    else
        return false;
}

bool MemoryCopy::readIn(uint64_t addr, size_t size)
{
    if (size)
    {
        this->_size = size;
        if (size > this->_bufferSize)
            resize(size);
    }
    this->_chainAddr = PointerEndpoint(addr);
    return readIn();
}

bool MemoryCopy::readIn(PointerEndpoint chainAddr, size_t size)
{
    if (size)
    {
        this->_size = size;
        if (size > this->_bufferSize)
            resize(size);
    }
    this->_chainAddr = chainAddr;
    return readIn();
}

bool MemoryCopy::writeBack()
{
    if (this->_chainAddr.valid())
        return this->_memory->write(this->_chainAddr, this->_size, this->_buffer.get());
    else
        return false;
}

bool MemoryCopy::writeBack(uint64_t addr)
{
    this->_chainAddr = PointerEndpoint(addr);
    return writeBack();
}

bool MemoryCopy::writeBack(PointerEndpoint chainAddr)
{
    this->_chainAddr = chainAddr;
    return writeBack();
}

bool MemoryCopy::readFromBuffer(uint64_t offset, size_t size, void* buffer) const
{
    if (this->_buffer == NULL)
        return false;

    if (offset + size <= this->_size)
    {
        memcpy(buffer, getBuffer() + offset, size);
        return true;
    }
    return false;
}

bool MemoryCopy::writeToBuffer(uint64_t offset, size_t size, void* buffer)
{
    if (this->_buffer == NULL)
        return false;

    if (offset + size <= this->_size)
    {
        memcpy(getBuffer() + offset, buffer, size);
        return true;
    }
    return false;
}

uint64_t MemoryCopy::translate(const void* ptr) const
{
    if (this->_buffer.get() <= ptr && ptr < this->_buffer.get() + this->_size)
        return ((uint64_t)ptr - (uint64_t)this->_buffer.get()) + this->_chainAddr.get();
    return 0;
}

std::vector<uint64_t> MemoryCopy::translate(const std::vector<void*>& ptrs) const
{
    std::vector<uint64_t> results;
    results.reserve(ptrs.size());
    for (void* ptr : ptrs)
    {
        results.push_back(translate(ptr));
    }
    return results;
}


}