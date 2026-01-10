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


}