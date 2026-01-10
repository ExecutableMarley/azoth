#pragma once

#include <stdint.h>
#include <algorithm>
#include <iomanip>

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

    void align(uint64_t align)
    {
        this->startAddr.alignDown(align);
        this->stopAddr.alignUp(align);
    }

    bool isAligned(uint64_t align) const
    {
        return this->startAddr.isAligned(align) && this->stopAddr.isAligned(align);
    }

    MemoryRange boundingRange(const MemoryRange& other) const
    {
        return MemoryRange(std::min(this->startAddr, other.startAddr), std::max(this->stopAddr, other.stopAddr));
    }

    MemoryRange overlapRange(const MemoryRange& other) const
    {
        Address start = std::max(startAddr, other.startAddr);
        Address stop  = std::min(stopAddr,  other.stopAddr);

        if (stop <= start)
            return MemoryRange{}; // invalid

        return MemoryRange(start, stop);
    }

    //Schnitt
    //Overlap

    // Todo: Remove or replace
    static MemoryRange max_range_32bit()
    {
        return MemoryRange(Address::minAddr(), Address::maxAddr32());
    }

    static MemoryRange max_range_64bit()
    {
        return MemoryRange(Address::minAddr(), Address::maxAddr());
    }
};

inline std::ostream& operator<<(std::ostream& os, const MemoryRange& range)
{
    if (!range.valid())
        return os << "MemoryRange{ invalid }";

    return os << "MemoryRange{ "
              << "start="   << range.startAddr
              << ", stop="  << range.stopAddr
              << ", size="  << range.size()
              << " }";
}

} // namespace Azoth