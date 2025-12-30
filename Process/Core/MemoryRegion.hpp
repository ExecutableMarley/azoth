#pragma once

#include <stdint.h>

#include "../Types/Address.hpp"
#include "../Types/MemoryRange.hpp"
#include "../Types/EMemoryProtection.hpp"
#include "../Types/EMemoryState.hpp"
#include "../Types/EMemoryType.hpp"

namespace Azoth
{


/**
 * @brief Describes a contiguous virtual memory region within a process.
 *
 * A MemoryRegion combines address range information with protection,
 * allocation state, and backing type in a platform-independent manner.
 */
struct MemoryRegion
{
    /**
     * @brief Base (starting) address of the memory region.
     */
    uint64_t baseAddress;

    /**
     * @brief Size of the memory region in bytes.
     */
    size_t   regionSize;

    /**
     * @brief Access protection flags of the memory region.
     */
    EMemoryProtection protection;

    /**
     * @brief Allocation state of the memory region.
     */
    EMemoryState state;

    /**
     * @brief Backing type of the memory region.
     */
    EMemoryType type;

    /**
     * @brief Construct an empty, invalid memory region.
     */
    MemoryRegion() : baseAddress(0), regionSize(0), protection(EMemoryProtection::None), state(EMemoryState::Unknown), type(EMemoryType::Unknown) {}

    /**
     * @brief Construct a fully-specified memory region.
     */
    MemoryRegion(uint64_t baseAddress, size_t regionSize, EMemoryProtection protection, EMemoryState state, EMemoryType memoryType)
    {
        this->baseAddress = baseAddress;
        this->regionSize = regionSize;
        this->protection = protection;
        this->state = state;
        this->type = memoryType;
    }

    /**
     * @brief Check whether the Region represents a valid mapping.
     *
     * @return True if baseAddress is non-zero and size is non-zero.
     */
	bool valid() const noexcept { return baseAddress != 0 && regionSize != 0; }

    /**
     * @brief Check whether the region allows read access.
     */
    bool read() const
    {
        return (this->protection & EMemoryProtection::Read) != EMemoryProtection::None;
    }

    /**
     * @brief Check whether the region allows write access.
     */
    bool write() const
    {
        return (this->protection & EMemoryProtection::Write) != EMemoryProtection::None;
    }

    /**
     * @brief Check whether the region allows execution.
     */
    bool exec() const
    {
        return (this->protection & EMemoryProtection::Execute) != EMemoryProtection::None;
    }

    /**
     * @brief Check whether the memory region is committed.
     * 
     * A committed region is backed by physical storage and can potentially
     * be accessed, subject to its protection flags.
     */
    bool committed() const
    {
        return this->state == EMemoryState::Committed;
    }

    /**
     * @brief Convert the memory region to its corresponding memory range.
     *
     * The returned range uses half-open interval semantics:
     * [baseAddress, baseAddress + regionSize).
     */
	operator MemoryRange() const { return MemoryRange(baseAddress, baseAddress + regionSize); };
};


} // namespace Azoth