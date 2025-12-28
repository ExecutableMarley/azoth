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