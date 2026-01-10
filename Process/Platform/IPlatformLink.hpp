/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <vector>
#include <string>

#include "EPlatformError.hpp"
#include "../Types/EProcessArchitecture.hpp"
#include "../Types/EProcessPrivilegeLevel.hpp"
#include "../Core/ProcessImage.hpp"
#include "../Core/MemoryRegion.hpp"

namespace Azoth
{

//Todo: All optional methods should return bool or int

/**
 * @brief Platform abstraction layer for process interaction.
 */
class IPlatformLink
{
	friend class CProcess;
	friend class CMemoryModule;
public:
	virtual ~IPlatformLink() = default;

	//=== Generic ===//

	virtual bool initialize()            = 0;
	virtual bool isInitialized() const   = 0;
	virtual bool attach(uint32_t procID) = 0;
	virtual bool isAttached() const      = 0;
	virtual void detach()                = 0;

	//=== Process Specific ===//

	virtual bool isAlive() const { return setError(EPlatformError::NotImplemented); }
	virtual bool terminate()     { return setError(EPlatformError::NotImplemented); }
	virtual bool suspend()       { return setError(EPlatformError::NotImplemented); }
	virtual bool resume()        { return setError(EPlatformError::NotImplemented); }
	virtual uint32_t getExitCode() const { return setError(EPlatformError::NotImplemented); }

	//=== Process Images ===//

	virtual ProcessImage getMainProcessImage() const { return ProcessImage(); };
	virtual ProcessImage getProcessImage(const std::string& name) const { return ProcessImage(); };
	virtual std::vector<ProcessImage> getAllProcessImages() const { return {}; }

	//=== Process Query ===//

	virtual uint32_t getProcessIDByName(const std::string& name) const = 0;
	virtual std::vector<uint32_t> getProcessIDsByName(const std::string& name) const = 0;
	virtual uint32_t getProcessIDByWindowName(const std::string& windowTitle) const = 0;
	virtual std::vector<uint32_t> getProcessIDsByWindowName(const std::string& windowTitle) const = 0;

	virtual bool getProcessName(uint32_t procID, std::string& name) const { return setError(EPlatformError::NotImplemented); }
	virtual bool getProcessPath(uint32_t procID, std::string& path) const { return setError(EPlatformError::NotImplemented); }
	virtual bool getProcessArchitecture(uint32_t procID, EProcessArchitecture& architecture)   const { return setError(EPlatformError::NotImplemented); }
	virtual bool getProcessPrivilegeLevel(uint32_t procID, EProcessPrivilegeLevel& privileges) const { return setError(EPlatformError::NotImplemented); }

	//=== Memory ===//

	virtual bool read(uint64_t addr, size_t size, void* buffer) const { return setError(EPlatformError::NotImplemented); }
	virtual bool write(uint64_t addr, size_t size, const void* buffer) { return setError(EPlatformError::NotImplemented); }

	virtual bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion) const { return setError(EPlatformError::NotImplemented); }

	virtual bool virtualProtect(uint64_t addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* oldProtect) { return setError(EPlatformError::NotImplemented); }
	
	virtual uint64_t virtualAllocate(uint64_t addr, size_t size, EMemoryProtection protection) { return setError(EPlatformError::NotImplemented); }
	
	virtual bool virtualFree(uint64_t addr) { return setError(EPlatformError::NotImplemented); }

	//=== Threads ===//

	bool isThreadAlive(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	bool suspendThread(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	bool resumeThread(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	bool terminateThread(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	bool joinThread(uint32_t threadID, uint32_t timeOutMillis) { return setError(EPlatformError::NotImplemented); }

	uint32_t getThreadExitCode(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	//set/get context

	//=== Error Handling ===//

protected:

	/**
	 * @brief Sets the last error state and returns a bool indicating platformError == Success
	 */
	bool setError(EPlatformError platformError, uint64_t osError = 0) const
	{
		_lastError.platformError = platformError;
		_lastError.osError       = osError;
		return platformError == EPlatformError::Success;
	}

	/**
	 * @brief Sets the last error state and returns a bool indicating platformError == Success
	 */
	bool setError(PlatformErrorState state) const
	{
		_lastError = state;
		return state.platformError == EPlatformError::Success;
	}

private:
	inline static thread_local PlatformErrorState _lastError;
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