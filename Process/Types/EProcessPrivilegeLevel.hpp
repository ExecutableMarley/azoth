/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <cstdint>

namespace Azoth
{


/**
 * @brief Represents the privilege level of a process.
 */
enum class EProcessPrivilegeLevel : std::uint8_t
{
	Unknown = 0,
	StandardUser,
	ElevatedUser,
	System,
};

constexpr bool isPrivileged(EProcessPrivilegeLevel level) noexcept
{
    return level == EProcessPrivilegeLevel::ElevatedUser || 
           level == EProcessPrivilegeLevel::System;
}

constexpr std::string_view to_string(EProcessPrivilegeLevel privilege)
{
    switch (privilege)
    {
        case EProcessPrivilegeLevel::Unknown:      return "Unknown";
        case EProcessPrivilegeLevel::StandardUser: return "StandardUser";
        case EProcessPrivilegeLevel::ElevatedUser: return "ElevatedUser";
        case EProcessPrivilegeLevel::System:       return "System";
        default: return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, EProcessPrivilegeLevel privilege)
{
    return os << to_string(privilege);
}


}