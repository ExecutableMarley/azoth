/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "AddressCursor.hpp"

#include "../ProcessModules/CMemoryModule.hpp"

#include <cstring>

namespace Azoth
{


bool AddressCursor::readBytes(Address addr, void *outBuf, size_t size) const
{
    if (_invalid)
        return false;
    // 1. Read from local cache if possible
    if (insideBuffer(addr, size))
    {
        const BYTE *src = _buffer.get() + (addr - _bufferStart);
        std::memcpy(outBuf, src, size);
        return true;
    }
    // 2. Fall back to dynamic reader if out of bounds
    if (_memory)
    {
        return _memory->read(addr, size, outBuf);
    }
    return false;
}


} // namespace Azoth