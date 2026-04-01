/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../Types/Address.hpp"
#include "../Types/MemoryRange.hpp"
#include "../Types/EMemoryProtection.hpp"
#include "../Types/EMemoryState.hpp"
#include "../Types/EMemoryType.hpp"

#include <cstdint>
#include <functional>
#include <ostream>
#include <sstream>
#include <iomanip>

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

/** @brief Writes a human-readable description of a MemoryRegion to a stream. */
inline std::ostream& operator<<(std::ostream& os, const MemoryRegion& region)
{
    if (!region.valid())
        return os << "MemoryRegion{ invalid }";

    Address base(region.baseAddress);
    Address end(region.baseAddress + region.regionSize);

    os << "MemoryRegion{ "
       << "range=[" << base << " - " << end << ")"
       << ", size=0x" << std::hex << region.regionSize
       << std::dec
       << " (" << region.regionSize << " bytes)"
       << ", prot=" << region.protection
       << ", state=" << region.state
       << ", type=" << region.type
       << " }";

    return os;
}

/** @brief Returns a human-readable string representation of a MemoryRegion. */
inline std::string to_string(const MemoryRegion& region)
{
    std::ostringstream oss;
    oss << region;
    return oss.str();
}

/**
 * Predicate used to filter memory regions.
 *
 * The function should return `true` if the given MemoryRegion should be
 * included in an operation, or `false` if it should be skipped.
 */
using MemoryRegionFilter = std::function<bool(const MemoryRegion&)>;


/** 
 * @brief Fine-grained filter for matching memory protection states.
 *
 * ProtectionFilter allows expressing requirements for individual protection
 * flags independently. Each permission can be:
 * 
 * - Ignore  : do not care about this flag
 * - Require : the flag must be present
 * - Forbid  : the flag must NOT be present
 */
class ProtectionFilter
{
public:
    /**
     * @brief Rule applied to a single protection flag.
     */
    enum ProtectionState : uint8_t
    {
        Ignore,
        Require,
        Forbid
    };

    /// Rule applied to the read permission.
    ProtectionState read    = ProtectionState::Ignore;
    /// Rule applied to the write permission.
    ProtectionState write   = ProtectionState::Ignore;
    /// Rule applied to the execute permission.
    ProtectionState execute = ProtectionState::Ignore;

    /// Constructs a filter that ignores all permissions.
    constexpr ProtectionFilter() = default;

    /// Constructs a filter with explicit rules for read, write and execute.
    constexpr ProtectionFilter(ProtectionState r, ProtectionState w, ProtectionState e)
        : read(r), write(w), execute(e) {}

    /**
     * @brief Creates a filter requiring an exact protection match.
     *
     * All permissions present in `prot` are required and all others are forbidden.
     */
    static constexpr ProtectionFilter exact(EMemoryProtection prot)
    {
        return {
            (prot & EMemoryProtection::Read)    != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Forbid,
            (prot & EMemoryProtection::Write)   != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Forbid,
            (prot & EMemoryProtection::Execute) != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Forbid
        };
    }

    /**
     * @brief Creates a filter that requires the specified permissions.
     *
     * Permissions not included in `prot` are ignored.
     */
    static constexpr ProtectionFilter require(EMemoryProtection prot)
    {
        return {
            (prot & EMemoryProtection::Read)    != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Ignore,
            (prot & EMemoryProtection::Write)   != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Ignore,
            (prot & EMemoryProtection::Execute) != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Ignore
        };
    }

    /**
     * @brief Creates a filter from a simple integer mask.
     *
     * Each parameter controls the rule for a permission:
     *
     * - `>0` → Require
     * - `<0` → Forbid
     * - `0`  → Ignore
     *
     * Order: (read, write, execute).
     */
    static constexpr ProtectionFilter mask(int8_t r, int8_t w, int8_t e)
    {
        auto conv = [](int8_t v) constexpr -> ProtectionState {
            return (v > 0) ? ProtectionState::Require :
                   (v < 0) ? ProtectionState::Forbid :
                             ProtectionState::Ignore;
            };
        return { conv(r), conv(w), conv(e) };
    }

    /// Require the region to be readable.
    constexpr ProtectionFilter& requireRead()  { read    = ProtectionState::Require; return *this; }
    /// Require the region to NOT be readable.
    constexpr ProtectionFilter& forbidRead()   { read    = ProtectionState::Forbid;  return *this; }
    /// Require the region to be writable.
    constexpr ProtectionFilter& requireWrite() { write   = ProtectionState::Require; return *this; }
    /// Require the region to NOT be writable.
    constexpr ProtectionFilter& forbidWrite()  { write   = ProtectionState::Forbid;  return *this; }
    /// Require the region to be executable.
    constexpr ProtectionFilter& requireExec()  { execute = ProtectionState::Require; return *this; }
    /// Require the region to NOT be executable.
    constexpr ProtectionFilter& forbidExec()   { execute = ProtectionState::Forbid;  return *this; }

    /**
     * @brief Checks a single permission rule against an actual protection mask.
     */
    constexpr bool checkProtection(ProtectionState filterState, EMemoryProtection actual, EMemoryProtection flag) const
    {
        bool isPresent = (actual & flag) != EMemoryProtection::None;

        if (filterState == ProtectionState::Require)
        {
            return isPresent;
        }
        else if (filterState == ProtectionState::Forbid)
        {
            return !isPresent;
        }
        return true;
    }

    /**
     * @brief Tests whether a protection mask satisfies this filter.
     */
    constexpr bool matchesProtection(EMemoryProtection actual) const
    {
        if (!checkProtection(read, actual, EMemoryProtection::Read))
        {
            return false;
        }

        if (!checkProtection(write, actual, EMemoryProtection::Write))
        {
            return false;
        }

        if(!checkProtection(execute, actual, EMemoryProtection::Execute))
        {
            return false;
        }

        return true;
    }

    /**
     * @brief Predicate operator for MemoryRegion filtering.
     *
     * Allows using ProtectionFilter directly with algorithms expecting a
     * region predicate.
     */
    constexpr bool operator()(const MemoryRegion& r) const
    {
        return matchesProtection(r.protection);
    }
};


} // namespace Azoth