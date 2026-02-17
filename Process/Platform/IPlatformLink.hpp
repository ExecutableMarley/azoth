/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <memory>

#include "EPlatformError.hpp"
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

	virtual bool isAlive(bool& isAliveState)     const { return setError(EPlatformError::NotImplemented); }
	virtual bool terminate()     { return setError(EPlatformError::NotImplemented); }
	virtual bool suspend()       { return setError(EPlatformError::NotImplemented); }
	virtual bool resume()        { return setError(EPlatformError::NotImplemented); }
	virtual bool getExitCode(uint32_t& exitCode) const { return setError(EPlatformError::NotImplemented); }

	//=== Process Images ===//

	virtual bool getMainProcessImage(ProcessImage& processImage) const { return setError(EPlatformError::NotImplemented); }
	virtual bool getProcessImage(const std::string& name, ProcessImage& processImage) const { return setError(EPlatformError::NotImplemented); }
	virtual bool getAllProcessImages(std::vector<ProcessImage>& processImages) const { return setError(EPlatformError::NotImplemented); }

	//=== Process Query ===//

	virtual bool getProcessIDByName(const std::string& name, uint32_t& procID) const { return setError(EPlatformError::NotImplemented); }
	virtual bool getProcessIDsByName(const std::string& name, std::vector<uint32_t>& procIDs) const { return setError(EPlatformError::NotImplemented); }
	virtual bool getProcessIDByWindowName(const std::string& windowTitle, uint32_t& procID) const { return setError(EPlatformError::NotImplemented); }

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

	virtual bool isThreadAlive(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	virtual bool suspendThread(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	virtual bool resumeThread(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	virtual bool terminateThread(uint32_t threadID) { return setError(EPlatformError::NotImplemented); }

	virtual bool joinThread(uint32_t threadID, uint32_t timeOutMillis) { return setError(EPlatformError::NotImplemented); }

	virtual bool getThreadExitCode(uint32_t threadID, uint32_t& exitCode) { return setError(EPlatformError::NotImplemented); }

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
 	* azoth operation executed on the current thread that failed.
	*
 	* The error state is stored in a thread-local context and is independent of
 	* any specific object instance
	* @note The returned value is only meaningful if the immediately preceding
 	*       platform link operation returned `false`. On successful calls, the
 	*       error state should be considered to be undefined.
	*/
	PlatformErrorState getLastError() const
	{
		return _lastError;
	}

	static PlatformErrorState getLast()
	{
		return _lastError;
	}
};

namespace Error
{

/**
 * @brief Returns detailed information about the last platform error that occurred
 *        in the calling thread.
 *
 * This function retrieves the error state associated with the most recent
 * azoth operation executed on the current thread that failed.
 *
 * The error state is stored in a thread-local context and is independent of
 * any specific object instance
 *
 * @note The returned value is only meaningful if the immediately preceding
 *       platform link operation returned `false`. On successful calls, the
 *       error state should be considered to be undefined.
 */
inline PlatformErrorState getLast()
{
	return IPlatformLink::getLast();
}

}


namespace Platform
{

std::unique_ptr<IPlatformLink> createDefaultLayer();

uint32_t getPID();

}


}