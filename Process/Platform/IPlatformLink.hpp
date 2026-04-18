/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

#include "EPlatformError.hpp"
#include "../Types/Address.hpp"
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

	virtual bool getExportSymbols(const ProcessImage& image, std::vector<ImageSymbol>& symbols) const { return setError(EPlatformError::NotImplemented); }
	virtual bool getImportSymbols(const ProcessImage& image, std::vector<ImageSymbol>& symbols) const { return setError(EPlatformError::NotImplemented); }

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
	 * @brief Handles a platform operation result and optionally checks for process termination.
	 *
	 * This function is a convenience wrapper around setError() that records the error state
	 * and can additionally detect whether the target process has terminated and set the 
	 * internal "dead" state. To avoid excessive system calls, isAlive() checks are throttled
	 * and only performed if at least ALIVE_CHECK_INTERVAL milliseconds have elapsed since
	 * the last check.
	 *
	 * @param platformError The error code from the platform operation.
	 * @param osError       Optional OS-specific error detail (default: 0).
	 * @param exitCheck     If true, performs a throttled process alive check on failure (default: false).
	 *
	 * @return The result of setError(); true if platformError == Success, false otherwise.
	 */
	bool handleFailure(EPlatformError platformError, uint64_t osError = 0, bool exitCheck = false) const
	{
		if (platformError == EPlatformError::Success) return setError(platformError, osError);

		if (exitCheck && !this->_isDead)
		{
			auto now = std::chrono::steady_clock::now();
			if ((now - _lastAliveCheck) >= ALIVE_CHECK_INTERVAL)
			{
				_lastAliveCheck = now;
				bool isAliveResult;
				if (isAlive(isAliveResult) && !isAliveResult)
				{
					_isDead = true;
				}
			}
		}
		return setError(platformError, osError);
	}

public:
	void setProcessToAlive() //Todo: This should get called automatically at some point
	{
		this->_isDead = false;
	}

	bool hasDied() const
	{
		return this->_isDead;
	}

public:
	/**
	 * @brief Sets the last error state and returns a bool indicating platformError == Success
	 */
	static bool setError(EPlatformError platformError, uint64_t detail = 0)
	{
		_lastError.platformError = platformError;
		_lastError.osError       = detail;
		return platformError == EPlatformError::Success;
	}

	/**
	 * @brief Sets the last error state and returns a bool indicating platformError == Success
	 */
	static bool setError(PlatformErrorState state)
	{
		_lastError = state;
		return state.platformError == EPlatformError::Success;
	}

private:
	mutable bool _isDead = true;
	mutable std::chrono::steady_clock::time_point _lastAliveCheck = std::chrono::steady_clock::time_point::min();
	static constexpr std::chrono::milliseconds ALIVE_CHECK_INTERVAL{500};

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