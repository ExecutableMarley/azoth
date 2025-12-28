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

	virtual bool isAlive()   { return false; }
	virtual bool terminate() { return false; }
	virtual bool suspend()   { return false; }
	virtual bool resume()    { return false; }
	virtual uint32_t getExitCode() { return 0; }

	//=== Process Images ===//

	virtual ProcessImage getMainProcessImage() { return ProcessImage(); };
	virtual ProcessImage getProcessImage(const std::string& name) { return ProcessImage(); };
	virtual std::vector<ProcessImage> getAllProcessImages() { return {}; }

	//=== Process Query ===//

	virtual uint32_t getProcessIDByName(const std::string& name) = 0;
	virtual std::vector<uint32_t> getProcessIDsByName(const std::string& name) = 0;
	virtual uint32_t getProcessIDByWindowName(const std::string& windowTitle) = 0;
	virtual std::vector<uint32_t> getProcessIDsByWindowName(const std::string& windowTitle) = 0;

	virtual bool getProcessName(uint32_t procID, std::string& name) { return false; }
	virtual bool getProcessPath(uint32_t procID, std::string& path) { return false; }
	virtual EProcessArchitecture getProcessArchitecture(uint32_t procID)     { return EProcessArchitecture::Unknown; }
	virtual EProcessPrivilegeLevel getProcessPrivilegeLevel(uint32_t procID) { return EProcessPrivilegeLevel::Unknown; }

	//=== Memory ===//

	virtual bool read(uint64_t addr, size_t size, void* buffer)  { return false; }
	virtual bool write(uint64_t addr, size_t size, const void* buffer) { return false; }

	virtual bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion) { return false; }

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

	/*
	None,
	Not implemented
	Access Denied

	OS Specific error
	*/
	//Todo: Implement error code system
	enum class EPlatformError : uint32_t
	{
    	Success = 0,
    	NotImplemented,    // 
    	NotSupported,      // The OS physically cannot do this
    	AccessDenied,      // Permission issue
    	InternalError,     // OS-specific error occurred
    	InvalidArgument    // Bad arguments passed
	};
};


}