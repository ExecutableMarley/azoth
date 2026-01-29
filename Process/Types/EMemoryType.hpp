/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>

namespace Azoth
{


/**
 * @brief Represents abstract memory region types.
 *
 * Describes the origin or backing type of a memory region in a portable manner.
 * This enum captures common concepts shared across operating systems.
 *
 * The numeric values intentionally match Windows MEM_* constants
 */
enum class EMemoryType : uint32_t
{
    Unknown = 0,
    Private = 0x20000,   // MEM_PRIVATE
    Mapped  = 0x40000,   // MEM_MAPPED
    Image   = 0x1000000, // MEM_IMAGE
};

// --- Helper Function ---

constexpr std::string_view to_string(EMemoryType type)
{
    switch (type)
    {
        case EMemoryType::Unknown: return "Unknown";
        case EMemoryType::Private: return "Private";
        case EMemoryType::Mapped:  return "Mapped";
        case EMemoryType::Image:   return "Image";
        default: return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, EMemoryType type)
{
    return os << to_string(type);
}


} // namespace Azoth