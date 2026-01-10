/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "PointerEndpoint.hpp"

#include "../ProcessModules/CMemoryModule.hpp"

namespace Azoth
{


//
void PtrChainLink::update()
{
    if (_parent)
        _mem->read(_parent->get() + _addr, this->_ptr);
    else
        _mem->read(_addr, this->_ptr);

    //_timer.reset();
}

//
uint64_t PtrChainLink::get()
{
    bool isTarget64Bit = true;

    if (_parent)
    {
        if (true)
        //if (_timer.checkReset(_mem->getPtrChainCacheDuration()))
            _mem->read(_parent->get() + _addr, isTarget64Bit ? 0x08 : 0x04, &_ptr);
        return _ptr;
    }
    else
    {
        if (true)
        //if (_timer.checkReset(_mem->getPtrChainCacheDuration()))
            _mem->read(_addr, isTarget64Bit ? 0x08 : 0x04, &_ptr);
        return _ptr;
    }
}

//
uint64_t PointerEndpoint::get() const
{
    if (_previous)
        return _previous->get() + _addOffset;
    return _addOffset;
}


}