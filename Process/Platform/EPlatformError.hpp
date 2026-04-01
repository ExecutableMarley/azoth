/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <ostream>

namespace Azoth
{


/**
 * @brief Platform-agnostic error codes.
 *
 * @note These values are stable and backend-independent.
 */
enum class EPlatformError : std::uint32_t
{
	Success = 0,
	NotImplemented,    // Platform backend does not implement this feature
	NotSupported,      // this OS Backend fundamentally cannot support this feature
	InvalidState,      // Wrong order of operations attempted
    OperationTimeout,  // The operation timed out
	SymbolNotFound,    // Required OS symbol could not be resolved
	ResourceNotFound,  // Process, File or Image not found
	//EntryNotFound,        // Specific tracking entry was not found
    ProcessLost,       // Target was found and attached, but has since exited
    InvalidArgument,   // Bad arguments passed
	AccessDenied,      // Permission issue
	RestorationViolation, // Operation would create overlapping or conflicting restoration points.
    MalformedData,     // Unexpected or invalid data was found
    DecodeError,       // osError contains ZyanStatus
    InternalError      // OS-specific error occurred. osError contains os error code
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
	std::uint64_t osError       = 0;
};


constexpr std::string_view to_string(EPlatformError err) noexcept
{
    using enum EPlatformError;

    switch (err)
    {
        case Success:              return "Success";
        case NotImplemented:       return "NotImplemented";
        case NotSupported:         return "NotSupported";
        case InvalidState:         return "InvalidState";
        case OperationTimeout:     return "OperationTimeout";
        case SymbolNotFound:       return "SymbolNotFound";
        case ResourceNotFound:     return "ResourceNotFound";
        case ProcessLost:          return "ProcessLost";
        case InvalidArgument:      return "InvalidArgument";
        case AccessDenied:         return "AccessDenied";
        case RestorationViolation: return "RestorationViolation";
        case MalformedData:        return "MalformedData";
        case DecodeError:          return "DecodeError";
        case InternalError:        return "InternalError";
    }

    return "UnknownPlatformError";
}

inline std::ostream& operator<<(std::ostream& os, EPlatformError err)
{
    return os << to_string(err);
}


inline std::ostream& operator<<(std::ostream& os, const PlatformErrorState& state)
{
    os << state.platformError;

    if (state.platformError == EPlatformError::InternalError)
    {
        os << " (osError=" << state.osError << ")";
    }

    return os;
}


}