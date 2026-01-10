/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>

namespace Azoth
{


/**
 * @brief Represents abstract memory allocation states.
 */
enum class EMemoryState : uint32_t
{
    Unknown = 0,
    Free,       
    Reserved,   
    Committed
};


} // namespace Azoth