/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>

namespace Azoth
{


/**
 * @brief Platform-agnostic error codes.
 *
 * @note These values are stable and backend-independent.
 */
enum class EPlatformError : uint32_t
{
	Success = 0,
	NotImplemented,    // Platform backend does not implement this feature
	NotSupported,      // this OS Backend fundamentally cannot support this feature
	InvalidState,      // Wrong order of operations attempted
	SymbolNotFound,    // Required OS symbol could not be resolved
	ResourceNotFound,  // Process, File or Image not found
	InvalidArgument,   // Bad arguments passed
	AccessDenied,      // Permission issue
	InternalError      // OS-specific error occurred
};


/**
 * @brief Detailed error state returned by a platform backend.
 *
 * Combines a platform-independent error code with an optional
 * OS-specific error value.
 */
struct PlatformErrorState
{
	/**
     * @brief High-level, platform-agnostic error code.
     */
	EPlatformError platformError = EPlatformError::Success;

	/**
     * @brief OS-specific error code.
     *
	 * This field is only set and should only be interpreted when
     * @ref platformError == @ref EPlatformError::InternalError.
	 * 
	 * Typical sources:
     * - Windows: GetLastError(), NTSTATUS
     * - Linux: errno
	 */
	uint64_t osError       = 0;
};


}