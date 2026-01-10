#pragma once

#include <stdint.h>

namespace Azoth
{

/**
 * @brief Represents memory protection flags.
 *
 * The enum is intended to be used as a bitmask.
 */
enum class EMemoryProtection : uint32_t
{
    None      = 0,
    Read      = 1 << 0,
    Write     = 1 << 1,
    Execute   = 1 << 2,
    ReadWrite = Read | Write,
    ReadExec  = Read | Execute,
    RWX       = Read | Write | Execute
};

// --- Bitwise Operators ---

constexpr EMemoryProtection operator|(EMemoryProtection a, EMemoryProtection b)
{
    return static_cast<EMemoryProtection>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr EMemoryProtection operator&(EMemoryProtection a, EMemoryProtection b)
{
    return static_cast<EMemoryProtection>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr EMemoryProtection operator~(EMemoryProtection a)
{
    return static_cast<EMemoryProtection>(~static_cast<uint32_t>(a));
}

constexpr EMemoryProtection& operator|=(EMemoryProtection& a, EMemoryProtection b)
{
    return a = a | b;
}

constexpr EMemoryProtection& operator&=(EMemoryProtection& a, EMemoryProtection b)
{
    return a = a & b;
}

// --- Helper Function ---

constexpr bool hasFlag(EMemoryProtection value, EMemoryProtection flag)
{
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag);
}

/* @class ProtectionFilter
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
    enum ProtectionState : uint8_t
    {
        Ignore,
        Require,
        Forbid
    };

    ProtectionState read    = ProtectionState::Ignore;
    ProtectionState write   = ProtectionState::Ignore;
    ProtectionState execute = ProtectionState::Ignore;

    constexpr ProtectionFilter() = default;

    constexpr ProtectionFilter(ProtectionState r, ProtectionState w, ProtectionState e)
        : read(r), write(w), execute(e) {}

    static constexpr ProtectionFilter exact(EMemoryProtection prot)
    {
        return {
            (prot & EMemoryProtection::Read)    != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Forbid,
            (prot & EMemoryProtection::Write)   != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Forbid,
            (prot & EMemoryProtection::Execute) != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Forbid
        };
    }

    static constexpr ProtectionFilter require(EMemoryProtection prot)
    {
        return {
            (prot & EMemoryProtection::Read)    != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Ignore,
            (prot & EMemoryProtection::Write)   != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Ignore,
            (prot & EMemoryProtection::Execute) != EMemoryProtection::None ? ProtectionState::Require : ProtectionState::Ignore
        };
    }

    static constexpr ProtectionFilter mask(int8_t r, int8_t w, int8_t e)
    {
        auto conv = [](int8_t v) constexpr -> ProtectionState {
            return (v > 0) ? ProtectionState::Require :
                   (v < 0) ? ProtectionState::Forbid :
                             ProtectionState::Ignore;
            };
        return { conv(r), conv(w), conv(e) };
    }

    constexpr ProtectionFilter& requireRead()  { read    = ProtectionState::Require; return *this; }
    constexpr ProtectionFilter& forbidRead()   { read    = ProtectionState::Forbid;  return *this; }
    constexpr ProtectionFilter& requireWrite() { write   = ProtectionState::Require; return *this; }
    constexpr ProtectionFilter& forbidWrite()  { write   = ProtectionState::Forbid;  return *this; }
    constexpr ProtectionFilter& requireExec()  { execute = ProtectionState::Require; return *this; }
    constexpr ProtectionFilter& forbidExec()   { execute = ProtectionState::Forbid;  return *this; }

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
};


} // namespace Azoth