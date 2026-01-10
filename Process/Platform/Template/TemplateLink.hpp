/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../IPlatformLink.hpp"

namespace Azoth
{


class TemplateToRename : IPlatformLink
{
public:
	~TemplateToRename() override
    {

    }

	//=== Generic ===//

	bool initialize();

	bool isInitialized();

	bool attach(uint32_t procID);

	bool isAttached() const;

	void detach()
    {

    }

	//=== Process Specific ===//

	bool isAlive() const
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool terminate()
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool suspend()
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool resume()
    {
        return setError(EPlatformError::NotImplemented);
    }

	uint32_t getExitCode() const
    {
        return setError(EPlatformError::NotImplemented);
    }

	//=== Process Images ===//

	ProcessImage getMainProcessImage() const
    {
        return ProcessImage();
    }

	ProcessImage getProcessImage(const std::string& name) const
    {
        return ProcessImage();
    }

	std::vector<ProcessImage> getAllProcessImages() const
    {
        return {};
    }

	//=== Process Query ===//

	uint32_t getProcessIDByName(const std::string& name) const;

	std::vector<uint32_t> getProcessIDsByName(const std::string& name) const;

	uint32_t getProcessIDByWindowName(const std::string& windowTitle) const;

	std::vector<uint32_t> getProcessIDsByWindowName(const std::string& windowTitle) const;

	bool getProcessName(uint32_t procID, std::string& name) const
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool getProcessPath(uint32_t procID, std::string& path) const
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool getProcessArchitecture(uint32_t procID, EProcessArchitecture& architecture) const
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool getProcessPrivilegeLevel(uint32_t procID, EProcessPrivilegeLevel& privileges) const
    {
        return setError(EPlatformError::NotImplemented);
    }

	//=== Memory ===//

	bool read(uint64_t addr, size_t size, void* buffer) const
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool write(uint64_t addr, size_t size, const void* buffer)
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion) const
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool virtualProtect(uint64_t addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* oldProtect)
    {
        return setError(EPlatformError::NotImplemented);
    }
	
	uint64_t virtualAllocate(uint64_t addr, size_t size, EMemoryProtection protection)
    {
        return setError(EPlatformError::NotImplemented);
    }
	
	bool virtualFree(uint64_t addr)
    {
        return setError(EPlatformError::NotImplemented);
    }

	//=== Threads ===//

	bool isThreadAlive(uint32_t threadID)
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool suspendThread(uint32_t threadID)
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool resumeThread(uint32_t threadID)
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool terminateThread(uint32_t threadID)
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool joinThread(uint32_t threadID, uint32_t timeOutMillis)
    {
        return setError(EPlatformError::NotImplemented);
    }

	uint32_t getThreadExitCode(uint32_t threadID)
    {
        return setError(EPlatformError::NotImplemented);
    }

	//set/get context
};


}