/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "PointerEndpoint.hpp"

#include "../ProcessModules/CMemoryModule.hpp"

namespace Azoth
{


//
void PtrChainLink::update(CMemoryModule& mem, std::chrono::milliseconds cacheDuration)
{
    Address base = 0;
    if (_parent)
    {
        base = _parent->get(mem, cacheDuration);
    }

    const uint64_t address = base + _addr;

    //ReadPtr
    bool success = mem.readPtr(address, _ptr);

    _lastUpdate = clock::now();
}

//
uint64_t PtrChainLink::get(CMemoryModule& mem, std::chrono::milliseconds cacheDuration)
{
    const auto now = clock::now();

    if (_lastUpdate.time_since_epoch().count() == 0 ||
        now - _lastUpdate >= cacheDuration)
    {
        update(mem, cacheDuration);
    }

    return _ptr;
}

uint64_t PointerEndpoint::get(std::chrono::milliseconds cacheDuration) const
{
    assert((bool)_mem != (bool)_previous);
    if (_mem && _previous)
    {
        return _previous->get(*_mem, cacheDuration) + _addOffset;
    }
    return _addOffset;
}

uint64_t PointerEndpoint::get() const
{
    return this->get(std::chrono::milliseconds(10));
}


}