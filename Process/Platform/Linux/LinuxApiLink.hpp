/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#if __linux__

#include "../IPlatformLink.hpp"

#include <limits.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

//Todo: Renaming to Unix naming scheme

namespace Azoth
{


namespace fs = std::filesystem;

class LinuxLink : public IPlatformLink
{ 
public:
	~LinuxLink() override
    {

    }

	//=== Generic ===//

	bool initialize() override
    {
        //[Required]
        return true;
    }

	bool isInitialized() const override
    {
        //[Required]
        return true;
    }

	bool attach(uint32_t procID) override
    {
        if (!procID) return setError(EPlatformError::InvalidArgument);
        this->_procID = procID;
        return true;
    }

	bool isAttached() const override
    {
        //[Required]
        return (bool)this->_procID;
    }

	void detach() override
    {
        this->_procID = 0;
    }

	//=== Process Specific ===//

	bool isAlive(bool& aliveState) const override
    {
        aliveState = false;
        if (!isAttached())
            return setError(EPlatformError::InvalidState);

        PlatformErrorState esp = sendSignal(this->_procID, 0);
        if (esp.platformError == EPlatformError::Success)
        {
            aliveState = true;
        }
        return setError(esp);
    }

	bool terminate() override
    {
        if (!isAttached())
            return setError(EPlatformError::InvalidState);

        return setError(sendSignal(this->_procID, SIGTERM));
    }

	bool suspend() override
    {
        if (!isAttached())
            return setError(EPlatformError::InvalidState);

        return setError(sendSignal(this->_procID, SIGSTOP));
    }

	bool resume() override
    {
        if (!isAttached())
            return setError(EPlatformError::InvalidState);

        return setError(sendSignal(this->_procID, SIGCONT));
    }

	bool getExitCode(uint32_t& exitCode) const override
    {
        return setError(EPlatformError::NotSupported);
    }

	//=== Process Images ===//

	bool getMainProcessImage(ProcessImage& processImage) const override
    {
        std::string path;
        bool success = getProcessName(_procID, path);
        if (!success)
            return false;

        return getProcessImage(path, processImage);
    }

	bool getProcessImage(const std::string& name, ProcessImage& processImage) const override
    {
        std::unordered_map<std::string, ProcessImage> procImageMap;
        auto errorCode = getProcessMappedBinaries(_procID, procImageMap);
        if (errorCode != EPlatformError::Success)
            return setError(errorCode);

        auto it = procImageMap.find(name);
        if (it == procImageMap.end())
            return setError(EPlatformError::ResourceNotFound);

        processImage = it->second;
        return setError(EPlatformError::Success);
    }

	bool getAllProcessImages(std::vector<ProcessImage>& processImages) const override
    {
        std::unordered_map<std::string, ProcessImage> procImageMap;
        auto errorCode = getProcessMappedBinaries(_procID, procImageMap);
        if (errorCode != EPlatformError::Success)
            return setError(errorCode);

        for (const auto& [key, img] : procImageMap)
        {
            processImages.push_back(img);
        }
        
        return setError(EPlatformError::Success);
    }

	//=== Process Query ===//

	bool getProcessIDByName(const std::string& name, uint32_t& procID) const override
    {
        std::vector<uint32_t> ids;
        if (!getProcessIDsByName(name, ids))
            return setError(EPlatformError::ResourceNotFound);

        procID = ids.front(); //Pick first
        return setError(EPlatformError::Success);
    }

	bool getProcessIDsByName(const std::string& name, std::vector<uint32_t>& procIDs) const override
    {
        procIDs.clear();

        for (const auto& entry : fs::directory_iterator("/proc"))
        {
            if (!entry.is_directory())
                continue;

            const std::string pidStr = entry.path().filename().string();
            if (!std::all_of(pidStr.begin(), pidStr.end(), ::isdigit))
                continue;

            std::ifstream commFile(entry.path() / "comm");
            if (!commFile)
                continue;

            std::string comm;
            std::getline(commFile, comm);

            if (comm == name)
                procIDs.push_back(static_cast<uint32_t>(std::stoul(pidStr)));
        }

        if (procIDs.empty())
            return setError(EPlatformError::ResourceNotFound);

        return setError(EPlatformError::Success);
    }

	bool getProcessIDByWindowName(const std::string& windowTitle, uint32_t& procID) const override
    {
        // No universal 'window to process' mapping on Linux
        // Override this method to provide a system specific implementation.
        return setError(EPlatformError::NotImplemented);
    }

	bool getProcessName(uint32_t procID, std::string& name) const override
    {
        name.clear();

        const std::string commPath = "/proc/" + std::to_string(procID) + "/comm";
        std::ifstream file(commPath);
        if (!file.is_open())
            return setError(EPlatformError::ResourceNotFound);

        std::getline(file, name);
        if (name.empty())
            return setError(EPlatformError::MalformedData);

        return setError(EPlatformError::Success);
    }

	bool getProcessPath(uint32_t procID, std::string& path) const override
    {
        path.clear();

        const std::string exePath = "/proc/" + std::to_string(procID) + "/exe";

        char buffer[PATH_MAX] = {};
        const ssize_t len = ::readlink(exePath.c_str(), buffer, sizeof(buffer) - 1);
        if (len <= 0)
            return setError(EPlatformError::ResourceNotFound);

        buffer[len] = '\0';
        path.assign(buffer);

        return true;
    }

	bool getProcessArchitecture(uint32_t procID, EProcessArchitecture& architecture) const override
    {
        return setError(EPlatformError::NotImplemented);
    }

	bool getProcessPrivilegeLevel(uint32_t procID, EProcessPrivilegeLevel& privileges) const override
    {
        return setError(EPlatformError::NotImplemented);
    }

	//=== Memory ===//

	bool read(uint64_t addr, size_t size, void* buffer) const override
    {
        if (!isAttached())
            return setError(EPlatformError::InvalidState);

        pid_t pid = static_cast<pid_t>(this->_procID);
        struct iovec local  = { buffer, size };
        struct iovec remote = { (void*)addr, size };

        ssize_t n = syscall(SYS_process_vm_readv, pid, &local, 1, &remote, 1, 0);
        if (n == (ssize_t)size) return setError(EPlatformError::Success);
        //if (n >= 0) Partial copy

        if (errno == ESRCH) return setError(EPlatformError::ResourceNotFound);
        if (errno == EPERM) return setError(EPlatformError::AccessDenied);
        return setError(EPlatformError::InternalError, (uint64_t)errno);
    }

	bool write(uint64_t addr, size_t size, const void* buffer) override
    {
        if (!isAttached())
            return setError(EPlatformError::InvalidState);

        pid_t pid = static_cast<pid_t>(this->_procID);
        struct iovec local  = { const_cast<void*>(buffer), size };
        struct iovec remote = { (void*)addr, size };

        ssize_t n = syscall(SYS_process_vm_writev, pid, &local, 1, &remote, 1, 0);
        if (n == (ssize_t)size) return setError(EPlatformError::Success);
        //if (n >= 0) Partial copy
        
        if (errno == ESRCH) return setError(EPlatformError::ResourceNotFound);
        if (errno == EPERM) return setError(EPlatformError::AccessDenied);
        return setError(EPlatformError::InternalError, (uint64_t)errno);
    }

	bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion) const
    {
        if (!isAttached())
            return setError(EPlatformError::InvalidState);

        pid_t pid = static_cast<pid_t>(this->_procID);
        std::string mapsPath = std::string("/proc/") + std::to_string(pid) + "/maps";
        std::ifstream maps(mapsPath);
        if (!maps.is_open())
            return setError(EPlatformError::InternalError, (uint64_t)errno);

        std::string line;
        while (std::getline(maps, line))
        {
            std::istringstream iss(line);
            std::string range, perms, offset, dev, inode, path;
            iss >> range >> perms >> offset >> dev >> inode;
            std::getline(iss, path);
            // leading space
            if (!path.empty() && path[0] == ' ') path.erase(0, 1);

            auto dash = range.find('-');
            if (dash == std::string::npos) continue;
            uint64_t start = std::stoull(range.substr(0, dash), nullptr, 16);
            uint64_t end   = std::stoull(range.substr(dash + 1), nullptr, 16);

            if (addr >= start && addr < end)
            {
                EMemoryProtection prot = EMemoryProtection::None;
                if (perms.size() >= 1 && perms[0] == 'r') prot |= EMemoryProtection::Read;
                if (perms.size() >= 2 && perms[1] == 'w') prot |= EMemoryProtection::Write;
                if (perms.size() >= 3 && perms[2] == 'x') prot |= EMemoryProtection::Execute;

                EMemoryType type = EMemoryType::Unknown;
                if (!path.empty() && path[0] != '[')
                {
                    if ((prot & EMemoryProtection::Execute) != EMemoryProtection::None)
                        type = EMemoryType::Image;
                    else
                        type = EMemoryType::Mapped;
                }
                else
                {
                    // anonymous/private mappings
                    if (perms.size() >= 4 && perms[3] == 'p')
                        type = EMemoryType::Private;
                    else
                        type = EMemoryType::Unknown;
                }

                memoryRegion.baseAddress = start;
                memoryRegion.regionSize  = static_cast<size_t>(end - start);
                memoryRegion.protection  = prot;
                memoryRegion.state       = EMemoryState::Committed;
                memoryRegion.type        = type;

                return setError(EPlatformError::Success);
            }
        }

        return setError(EPlatformError::ResourceNotFound);
    }

	bool virtualProtect(uint64_t addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* oldProtect)
    {
        //Todo: ptrace
        return setError(EPlatformError::NotImplemented);
    }
	
	uint64_t virtualAllocate(uint64_t addr, size_t size, EMemoryProtection protection)
    {
        //Todo: ptrace
        return setError(EPlatformError::NotImplemented);
    }
	
	bool virtualFree(uint64_t addr)
    {
        //Todo: ptrace
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

private:
    uint32_t _procID;
};


}

#endif