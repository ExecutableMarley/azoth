/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "CMemoryModule.hpp"

#include "../Types/EProcessArchitecture.hpp"
#include "../CProcess.hpp"

namespace Azoth
{


bool CMemoryModule::readPtr(uint64_t addr, uint64_t& out)
{
    const auto architecture = _backPtr->GetArchitecture();
    if (architecture == EProcessArchitecture::x86 || architecture == EProcessArchitecture::ARM32)
    {
        if (uint32_t tmp; read(addr, tmp))
        {
            out = tmp;
            return true;
        }
        return false;
    }
    else
    {
        return read(addr, out);
    }
}


}