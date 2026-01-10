/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>

namespace Azoth
{


/**
 * @brief Represents the privilege level of a process.
 */
enum class EProcessPrivilegeLevel : uint8_t
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

}