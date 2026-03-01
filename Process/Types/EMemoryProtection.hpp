/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <ostream>

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

constexpr std::string_view to_string(EMemoryProtection flags)
{
    switch (flags)
    {
        case EMemoryProtection::None:      return "None";
        case EMemoryProtection::Read:      return "Read";
        case EMemoryProtection::Write:     return "Write";
        case EMemoryProtection::Execute:   return "Execute";
        case EMemoryProtection::ReadWrite: return "ReadWrite";
        case EMemoryProtection::ReadExec:  return "ReadExec";
        case EMemoryProtection::RWX:       return "RWX";
        default: return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, EMemoryProtection flags)
{
    return os << to_string(flags);
}


} // namespace Azoth