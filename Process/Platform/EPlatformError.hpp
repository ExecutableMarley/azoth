#pragma once

#include <stdint.h>

namespace Azoth
{


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

struct PlatformErrorState
{
	EPlatformError platformError = EPlatformError::Success;
	uint64_t       osError       = 0;
};


}