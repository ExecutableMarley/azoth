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
#include <vector>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include "Utility/LinuxProcess.hpp"

//Todo: Renaming to Unix naming scheme

namespace Azoth
{


namespace fs = std::filesystem;

class LinuxLink : public IPlatformLink
{ 
public:
    LinuxLink()
        : _procID(0)
        , _lastCacheUpdate(std::chrono::steady_clock::time_point::min())
        , _lastMapsWriteTime(std::filesystem::file_time_type::min())
        , _cacheTTL(std::chrono::milliseconds(1000))
    {
    }

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
        // attach and invalidate cache
        this->_procID = procID;
        std::lock_guard<std::mutex> lock(this->_cacheMutex);
        this->_cachedMemoryRegions.clear();
        this->_cachedProcessImages.clear();
        this->_lastCacheUpdate = std::chrono::steady_clock::time_point::min();
        this->_lastMapsWriteTime = std::filesystem::file_time_type::min();
        return true;
    }

	bool isAttached() const override
    {
        //[Required]
        return (bool)this->_procID;
    }

	void detach() override
    {
        // detach and invalidate cache
        this->_procID = 0;
        std::lock_guard<std::mutex> lock(this->_cacheMutex);
        this->_cachedMemoryRegions.clear();
        this->_cachedProcessImages.clear();
        this->_lastCacheUpdate = std::chrono::steady_clock::time_point::min();
        this->_lastMapsWriteTime = std::filesystem::file_time_type::min();
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
        if (!ensureCacheValid())
            return setError(EPlatformError::ResourceNotFound);

        std::lock_guard<std::mutex> lock(this->_cacheMutex);
        auto it = this->_cachedProcessImages.find(name);
        if (it == this->_cachedProcessImages.end())
            return setError(EPlatformError::ResourceNotFound);

        processImage = it->second;
        return setError(EPlatformError::Success);
    }

	bool getAllProcessImages(std::vector<ProcessImage>& processImages) const override
    {
        if (!ensureCacheValid())
            return setError(EPlatformError::ResourceNotFound);

        std::lock_guard<std::mutex> lock(this->_cacheMutex);
        for (const auto& [key, img] : this->_cachedProcessImages)
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

#if 0
        ssize_t n = syscall(SYS_process_vm_readv, pid, &local, 1, &remote, 1, 0);
#else
        ssize_t n = process_vm_readv(pid, &local, 1, &remote, 1, 0);
#endif
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

#if 0
        ssize_t n = syscall(SYS_process_vm_writev, pid, &local, 1, &remote, 1, 0);
#else
        ssize_t n = process_vm_writev(pid, &local, 1, &remote, 1, 0);
#endif
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

        if (!ensureCacheValid())
            return setError(EPlatformError::ResourceNotFound);

        std::lock_guard<std::mutex> lock(this->_cacheMutex);

        // Todo: Linear time and also not clean
        const auto& entries = this->_cachedMemoryRegions;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            const auto& cur = entries[i];
            uint64_t curStart = cur.baseAddress;
            uint64_t curEnd   = cur.baseAddress + cur.regionSize;

            // Case 1: Address inside valid region
            if (addr >= curStart && addr < curEnd)
            {
                memoryRegion = cur;
                return setError(EPlatformError::Success);
            }

            // Case 2: Address before first region
            if (i == 0 && addr < curStart)
            {
                memoryRegion.baseAddress = 0;
                memoryRegion.regionSize  = static_cast<size_t>(curStart);
                memoryRegion.protection  = EMemoryProtection::None;
                memoryRegion.state       = EMemoryState::Free;
                memoryRegion.type        = EMemoryType::Unknown;
                return setError(EPlatformError::Success);
            }

            // Case 3: Gap between current and next region
            if (i + 1 < entries.size())
            {
                const auto& next = entries[i + 1];
                uint64_t nextStart = next.baseAddress;

                if (addr >= curEnd && addr < nextStart)
                {
                    memoryRegion.baseAddress = curEnd;
                    memoryRegion.regionSize  = static_cast<size_t>(nextStart - curEnd);
                    memoryRegion.protection  = EMemoryProtection::None;
                    memoryRegion.state       = EMemoryState::Free;
                    memoryRegion.type        = EMemoryType::Unknown;
                    return setError(EPlatformError::Success);
                }
            }

            // Case 4: After last region
            if (i == entries.size() - 1 && addr >= curEnd)
            {
                memoryRegion.baseAddress = curEnd;
                memoryRegion.regionSize  = static_cast<size_t>(
                    std::numeric_limits<uint64_t>::max() - curEnd);
                memoryRegion.protection  = EMemoryProtection::None;
                memoryRegion.state       = EMemoryState::Free;
                memoryRegion.type        = EMemoryType::Unknown;
                return setError(EPlatformError::Success);
            }
        }
        // Should never reach here
        return setError(EPlatformError::InvalidState); //Todo: Change error code
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
    // Ensure the cached maps and images are up-to-date.
    bool ensureCacheValid() const
    {
        if (!isAttached())
            return false;

        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(this->_cacheMutex);

        if (this->_lastCacheUpdate != std::chrono::steady_clock::time_point::min() && (now - this->_lastCacheUpdate) < this->_cacheTTL)
            return true;

        std::filesystem::file_time_type mapsTime;
        if (getMapsWriteTime(this->_procID, mapsTime))
        {
            if (mapsTime == this->_lastMapsWriteTime && this->_lastCacheUpdate != std::chrono::steady_clock::time_point::min())
            {
                this->_lastCacheUpdate = now;
                return true;
            }
        }

        // Refresh caches under lock
        EPlatformError res = this->_refreshCachesLocked();
        return (res == EPlatformError::Success);
    }

    // Refresh caches; MUST be called with _cacheMutex held
    EPlatformError _refreshCachesLocked() const
    {
        std::vector<MemoryRegion> regions;
        std::unordered_map<std::string, ProcessImage> images;

        EPlatformError err = parseProcessMaps(this->_procID, regions, images);
        if (err != EPlatformError::Success)
            return err;

        std::filesystem::file_time_type mapsTime;
        if (!getMapsWriteTime(this->_procID, mapsTime))
            mapsTime = std::filesystem::file_time_type::min();

        // swap into mutable caches
        this->_cachedMemoryRegions.swap(regions);
        this->_cachedProcessImages.swap(images);
        this->_lastMapsWriteTime = mapsTime;
        this->_lastCacheUpdate = std::chrono::steady_clock::now();

        return EPlatformError::Success;
    }
    uint32_t _procID;

    // Caching for memory regions and mapped binaries
    mutable std::mutex _cacheMutex;
    mutable std::vector<MemoryRegion> _cachedMemoryRegions;
    mutable std::unordered_map<std::string, ProcessImage> _cachedProcessImages;
    mutable std::chrono::steady_clock::time_point _lastCacheUpdate;
    mutable std::filesystem::file_time_type _lastMapsWriteTime;
    std::chrono::milliseconds _cacheTTL;
};


}

#endif