/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <cstdint>

namespace Azoth
{


/**
 * @brief Represents abstract memory allocation states.
 */
enum class EMemoryState : std::uint32_t
{
    Unknown = 0,
    Free,       
    Reserved,   
    Committed
};

// --- Helper Function ---

constexpr std::string_view to_string(EMemoryState state)
{
    switch (state)
    {
        case EMemoryState::Unknown:   return "Unknown";
        case EMemoryState::Free:      return "Free";
        case EMemoryState::Reserved:  return "Reserved";
        case EMemoryState::Committed: return "Committed";
        default: return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, EMemoryState state)
{
    return os << to_string(state);
}


} // namespace Azoth