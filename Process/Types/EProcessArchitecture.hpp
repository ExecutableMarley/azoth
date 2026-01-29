/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>

namespace Azoth
{


/**
 * @brief Represents the CPU instruction set architecture of a process.
 */
enum class EProcessArchitecture : uint8_t
{
	Unknown = 0,
	x86,
	x64,
	ARM32,
	ARM64,
};

/**
 * @brief Returns true if the architecture uses 64-bit pointers.
 */
constexpr bool isArchitecture64Bit(EProcessArchitecture arch) noexcept
{
    return arch == EProcessArchitecture::x64 || 
           arch == EProcessArchitecture::ARM64;
}

/**
 * @brief Returns true if the architecture uses 32-bit pointers.
 */
constexpr bool isArchitecture32Bit(EProcessArchitecture arch) noexcept
{
    return arch == EProcessArchitecture::x86 || 
           arch == EProcessArchitecture::ARM32;
}

constexpr std::string_view to_string(EProcessArchitecture arch)
{
    switch (arch)
    {
        case EProcessArchitecture::Unknown: return "Unknown";
        case EProcessArchitecture::x86:     return "x86";
        case EProcessArchitecture::x64:     return "x64";
        case EProcessArchitecture::ARM32:   return "ARM32";
		case EProcessArchitecture::ARM64:   return "ARM64";
        default: return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, EProcessArchitecture arch)
{
    return os << to_string(arch);
}


}