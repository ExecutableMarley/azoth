#pragma once

#include <stdint.h>
#include <vector>
#include <string>

#include "../Types/EProcessArchitecture.hpp"
#include "../Types/EProcessPrivilegeLevel.hpp"
#include "../Core/ProcessImage.hpp"
#include "../Core/MemoryRegion.hpp"

namespace Azoth
{


/**
 * @brief Platform abstraction layer for process interaction.
 */
class IPlatformLink
{
public:
	virtual ~IPlatformLink() = default;

	//=== Generic ===//

	virtual bool initialize()            = 0;
	virtual bool isInitialized() const   = 0;
	virtual bool attach(uint32_t procID) = 0;
	virtual bool isAttached() const      = 0;
	virtual void detach()                = 0;

	//=== Process Specific ===//

	virtual bool isAlive() const { return false; }
	virtual bool terminate()     { return false; }
	virtual bool suspend()       { return false; }
	virtual bool resume()        { return false; }
	virtual uint32_t getExitCode() const { return 0; }

	//=== Process Images ===//

	virtual ProcessImage getMainProcessImage() const { return ProcessImage(); };
	virtual ProcessImage getProcessImage(const std::string& name) const { return ProcessImage(); };
	virtual std::vector<ProcessImage> getAllProcessImages() const { return {}; }

	//=== Process Query ===//

	virtual uint32_t getProcessIDByName(const std::string& name) const = 0;
	virtual std::vector<uint32_t> getProcessIDsByName(const std::string& name) const = 0;
	virtual uint32_t getProcessIDByWindowName(const std::string& windowTitle) const = 0;
	virtual std::vector<uint32_t> getProcessIDsByWindowName(const std::string& windowTitle) const = 0;

	virtual bool getProcessName(uint32_t procID, std::string& name) const { return false; }
	virtual bool getProcessPath(uint32_t procID, std::string& path) const { return false; }
	virtual bool getProcessArchitecture(uint32_t procID, EProcessArchitecture& architecture)   const { return false; }
	virtual bool getProcessPrivilegeLevel(uint32_t procID, EProcessPrivilegeLevel& privileges) const { return false; }

	//=== Memory ===//

	virtual bool read(uint64_t addr, size_t size, void* buffer) const { return false; }
	virtual bool write(uint64_t addr, size_t size, const void* buffer) { return false; }

	virtual bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion) const { return false; }

	virtual bool virtualProtect(uint64_t addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* oldProtect) { return false; }
	
	virtual uint64_t virtualAllocate(uint64_t addr, size_t size, EMemoryProtection protection) { return false; }
	
	virtual bool virtualFree(uint64_t addr) { return false; }

	//=== Threads ===//

	bool isThreadAlive(uint32_t threadID) { return false; }

	bool suspendThread(uint32_t threadID) { return false; }

	bool resumeThread(uint32_t threadID) { return false; }

	bool terminateThread(uint32_t threadID) { return false; }

	bool joinThread(uint32_t threadID, uint32_t timeOutMillis) { return false; }

	uint32_t getThreadExitCode(uint32_t threadID) { return 0; }

	//set/get context

	//=== Error Handling ===//

	enum class EPlatformError : uint32_t
	{
    	Success = 0,
    	NotImplemented,    // Platform backend does not implement this feature
    	NotSupported,      // OS fundamentally cannot support this feature
		SymbolNotFound,    // Required OS symbol could not be resolved
    	InvalidArgument,   // Bad arguments passed
		AccessDenied,      // Permission issue
    	InternalError      // OS-specific error occurred
	};

	struct PlatformErrorState
	{
		EPlatformError platformError = EPlatformError::Success;
		uint64_t       osError       = 0;
	};
protected:
	/**
	 * @brief Sets the last error state and always returns false.
	 */
	bool setError(EPlatformError platformError, uint64_t osError = 0) const
	{
		_lastError.platformError = platformError;
		_lastError.osError       = osError;
		return false;
	}
private:
	static thread_local PlatformErrorState _lastError;
public:
	/**
 	* @brief Returns detailed information about the last error that occurred
 	*        in the calling thread.
	*
 	* This function returns the error state associated with the most recent
 	* platform link operation executed on the current thread that failed.
	*
	* @note The returned value is only meaningful if the immediately preceding
 	*       operation returned `false`. On successful calls, the error state is
 	*       not modified and should be considered undefined.
	*/
	PlatformErrorState getLastError() const
	{
		return _lastError;
	}
};


}