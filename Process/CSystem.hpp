/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "Types/EProcessArchitecture.hpp"
#include "Platform/IPlatformLink.hpp"

#include <string>
#include <chrono>
#include <thread>

namespace Azoth
{


struct ProcessInfo
{
    uint32_t pid;
    std::string name;
    std::string path;
    EProcessArchitecture arch;
};

class CSystem
{
public:

    CSystem(std::unique_ptr<IPlatformLink> platformLink) : _platformLink(std::move(platformLink))
    {
        assert(_platformLink);
    }

    CSystem(const CSystem&) = delete;

	CSystem& operator=(const CSystem&) = delete;

public:
    /**
	 * @brief Initialize the underlying platform backend.
	 *
	 * @return True if initialization succeeded.
	 *
	 * This must be called before any other operation.
	 */
    bool initialize()
	{
		return _platformLink->initialize();
	}

    /**
	 * @brief Check whether the platform backend is initialized.
	 */
	bool isInitialized() const
	{
		return _platformLink->isInitialized();
	}

    // --- Process enumeration ---
    //std::vector<ProcessInfo> enumerateProcesses() const;
    //std::vector<ProcessInfo> findProcesses(const ProcessQuery&) const;

    // --- Waiting ---
    
    std::optional<ProcessInfo> waitForProcess(
        const std::string& queryName,
        std::chrono::milliseconds timeout,
        std::chrono::milliseconds pollInterval = std::chrono::milliseconds(500)) const
    {
        uint32_t procID;
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout)
        {
            if (_platformLink->getProcessIDByName(queryName, procID))
            {
                std::string procName, procPath;
                EProcessArchitecture arch;
                _platformLink->getProcessName(procID, procName);
                _platformLink->getProcessPath(procID, procPath);
                _platformLink->getProcessArchitecture(procID, arch);
                return ProcessInfo{procID, procName, procPath, arch};
            }
            std::this_thread::sleep_for(pollInterval);
        }
        _platformLink->setError(EPlatformError::OperationTimeout);
        return std::nullopt;
    }

    // --- Lookup ID's ---

    uint32_t getProcessID(const std::string &name) const
    {
        uint32_t procID = 0;
        _platformLink->getProcessIDByName(name, procID);
        return procID;
    }

    std::vector<uint32_t> getProcessIDs(const std::string &name) const
    {
        std::vector<uint32_t> procIDs = {};
        _platformLink->getProcessIDsByName(name, procIDs);
        return procIDs;
    }

    uint32_t getProcessIDByWindow(const std::string &name) const
    {
        uint32_t procID = 0;
        _platformLink->getProcessIDByWindowName(name, procID);
        return procID;
    }

    // --- System info ---
    //uint32_t getOwnProcessID() const;
    //EProcessArchitecture getNativeArchitecture() const;
    //EProcessArchitecture getOwnProcessArchitecture() const

private:
    std::unique_ptr<IPlatformLink> _platformLink;
};


}