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


} // namespace Azoth