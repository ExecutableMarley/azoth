/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <algorithm>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <cassert>

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
    /** Inclusive start address of the range. */
    Address startAddr = 0;

    /** Exclusive end address of the range. */
    Address stopAddr = 0;

    /** @brief Constructs an empty (invalid) range. */
    MemoryRange() noexcept = default;

    /**
     * @brief Construct a memory range from explicit start and stop addresses.
     *
     * If @p stopAddr is smaller than @p startAddr, the range is clamped
     * to an empty range [startAddr, startAddr).
     *
     * @param startAddr Inclusive start address.
     * @param stopAddr  Exclusive end address.
     */
    MemoryRange(uint64_t startAddr, uint64_t stopAddr) noexcept : 
        startAddr(startAddr), stopAddr(stopAddr)
    {
        if (this->stopAddr < this->startAddr)
            this->stopAddr = this->startAddr;
    }

    /**
     * @brief Get the size of the range in bytes.
     *
     * @return Number of bytes covered by the range.
     *         Returns 0 for invalid or empty ranges.
     */
    size_t size() const { return stopAddr - startAddr; }

    /**
     * @brief Check whether the range is empty.
     */
    bool empty() const { return stopAddr <= startAddr; }

    /**
     * @brief Check whether the range is non-empty.
     *
     * A range is considered valid if @c stopAddr > @c startAddr.
     */
    bool valid() const { return stopAddr > startAddr; }

    /**
     * @brief Check whether an address lies within the range.
     * 
     * @param addr Address to test.
     */
    bool contains(Address addr) const
    {
        return startAddr <= addr && addr < stopAddr;
    }

    /**
     * @brief Check whether another range is fully contained within this range.
     *
     * @param other Range to test.
     */
    bool contains(const MemoryRange& other) const
    {
        return startAddr <= other.startAddr && other.stopAddr <= stopAddr;
    }

    /**
     * @brief Align the range to a given boundary.
     *
     * The start address is aligned downward, and the stop address
     * is aligned upward, potentially increasing the covered range.
     *
     * @param align Alignment value (must be a power of two).
     */
    void align(uint64_t align)
    {
        assert((align & (align - 1)) == 0 && "Alignment must be power of two");
        this->startAddr.alignDown(align);
        this->stopAddr.alignUp(align);
    }

    /**
     * @brief Check whether both range boundaries are aligned.
     *
     * @param align Alignment value (must be a power of two).
     * @return True if both start and stop addresses are aligned.
     */
    bool isAligned(uint64_t align) const
    {
        return this->startAddr.isAligned(align) && this->stopAddr.isAligned(align);
    }

    /**
     * @brief Compute the smallest range that fully contains this range
     *        and another range.
     *
     * @param other Another memory range.
     * @return Bounding range spanning both ranges.
     */
    MemoryRange boundingRange(const MemoryRange& other) const
    {
        return MemoryRange(std::min(this->startAddr, other.startAddr), std::max(this->stopAddr, other.stopAddr));
    }

    /**
     * @brief Compute the overlapping portion of this range and another range.
     *
     * @param other Another memory range.
     * @return Intersection of both ranges, or an empty range if they do not overlap.
     */
    MemoryRange overlapRange(const MemoryRange& other) const
    {
        Address start = std::max(startAddr, other.startAddr);
        Address stop  = std::min(stopAddr,  other.stopAddr);

        if (stop <= start)
            return MemoryRange{}; // invalid

        return MemoryRange(start, stop);
    }

    /**
     * @brief Maximum possible address range for a 32-bit address space.
     *
     * @return Range spanning the full 32-bit virtual address space.
     */
    static MemoryRange max_range_32bit()
    {
        return MemoryRange(Address::minAddr(), Address::maxAddr32());
    }

    /**
     * @brief Maximum possible address range for a 64-bit address space.
     *
     * @return Range spanning the full 64-bit virtual address space.
     */
    static MemoryRange max_range_64bit()
    {
        return MemoryRange(Address::minAddr(), Address::maxAddr());
    }
};

/**
 * @brief Stream insertion operator for MemoryRange.
 *
 * Produces a human-readable representation of the range, suitable for
 * logging and debugging.
 *
 * Format:
 *   - "MemoryRange{ invalid }" for empty or invalid ranges
 *   - "MemoryRange{ start=..., stop=..., size=... }" otherwise
 *
 * @param os    Output stream.
 * @param range Memory range to print.
 * @return Reference to the output stream.
 */
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

/**
 * @brief Convert a MemoryRange to a string.
 *
 * This is a convenience wrapper around the stream insertion operator.
 *
 * @param range Memory range to convert.
 * @return String representation of the range.
 */
inline std::string to_string(const MemoryRange& range)
{
    std::ostringstream oss;
    oss << range;
    return oss.str();
}


} // namespace Azoth