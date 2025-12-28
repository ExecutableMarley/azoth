#pragma once

#include <stdint.h>
#include <algorithm>

#include "Address.hpp"

#undef min
#undef max

namespace Azoth
{


/**
 * @brief Half-open virtual memory address range.
 *
 * Represents a contiguous memory interval [startAddr, stopAddr).
 * The start address is inclusive, the stop address is exclusive.
 *
 * The class does not guarantee that the range is valid or mapped.
 */
class MemoryRange
{
public:
    Address startAddr = 0;
    Address stopAddr = 0;

    MemoryRange() noexcept = default;

    MemoryRange(uint64_t startAddr, uint64_t stopAddr) noexcept : 
        startAddr(startAddr), stopAddr(stopAddr)
    {
        if (this->stopAddr < this->startAddr)
            this->stopAddr = this->startAddr;
    }

    size_t size() const { return stopAddr - startAddr; }

    bool valid() const { return stopAddr > startAddr; }

    //Union
    MemoryRange operator +(const MemoryRange& other) const
    {
        return MemoryRange(std::min(this->startAddr, other.startAddr), std::max(this->stopAddr, other.stopAddr));
    }

    //Align
    //Schnitt
    //Overlap
    //MaxRange

    //Todo: Handle this differently. This is not guaranteed to work

    static constexpr uint64_t minAddr()   { return 0x10000; }
    //Probably not correct for Linux 32bit
    static constexpr uint64_t maxAddr32() { return 0x7FFFFFFF; }
    static constexpr uint64_t maxAddr()   { return 0x7FFFFFFEFFFF; }

    // Todo: Remove or replace
    static MemoryRange max_range_32bit()
    {
        return MemoryRange(minAddr(), maxAddr32());
    }

    static MemoryRange max_range_64bit()
    {
        return MemoryRange(minAddr(), maxAddr());
    }
};


} // namespace Azoth