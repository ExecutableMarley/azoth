/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "PointerEndpoint.hpp"

#include "../ProcessModules/CMemoryModule.hpp"

namespace Azoth
{


//
bool PtrChainLink::update(CMemoryModule& mem, std::chrono::milliseconds cacheDuration)
{
    Address base = _parent ? _parent->get(mem, cacheDuration) : Address::null();

    if (_parent && !base)
    {
        _ptr = Address::null();
        return false;
    }

    const Address address = base + _addr;

    if (!mem.readPtr(address, _ptr))
    {
        _ptr = Address::null();
        _lastUpdate = clock::now();
        return false;
    }

    _lastUpdate = clock::now();
    return true;
}

//
Address PtrChainLink::get(CMemoryModule& mem, std::chrono::milliseconds cacheDuration)
{
    const auto now = clock::now();

    if (_lastUpdate.time_since_epoch().count() == 0 ||
        now - _lastUpdate >= cacheDuration)
    {
        update(mem, cacheDuration);
    }

    return _ptr;
}


//

Address PointerEndpoint::get(std::chrono::milliseconds cacheDuration) const
{
    assert((bool)_mem == (bool)_previous);
    if (_mem && _previous)
    {
        return _previous->get(*_mem, cacheDuration) + _addOffset;
    }
    return _addOffset;
}

Address PointerEndpoint::get() const
{
    //Todo: Use configurable default duration
    return this->get(std::chrono::milliseconds(10));
}


}