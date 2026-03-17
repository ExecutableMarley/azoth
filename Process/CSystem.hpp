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


/**
 * @brief Basic information about a running process.
 *
 * This structure represents a lightweight snapshot of process metadata
 * obtained from the operating system. It is typically returned by
 * process lookup or enumeration functions.
 */
struct ProcessInfo
{
    /// Process identifier assigned by the operating system.
    uint32_t pid;
    /// Executable name of the process (without path).
    std::string name;
    /// Full filesystem path to the executable.
    std::string path;
    /// Architecture of the process (e.g. x86, x64).
    EProcessArchitecture arch;
};

/**
 * @brief High-level interface for interacting with system processes.
 *
 * CSystem provides a platform-independent interface for common system
 * operations such as:
 *
 * - locating processes
 * - waiting for processes to start
 * - querying process metadata
 *
 * All platform-specific behavior is delegated to the provided
 * IPlatformLink implementation.
 *
 * Before using any functionality, the system backend must be initialized
 * via initialize().
 *
 * Error handling:
 * Most operations report failures through the global error system.
 * The last error can be retrieved using Error::getLast().
 */
class CSystem
{
public:

    /**
     * @brief Construct a system interface using a platform backend.
     *
     * @param platformLink Platform-specific implementation.
     * Ownership of the backend is transferred to the CSystem instance.
     * 
     * @note The CSystem object does not automatically initialize.
     */
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
    
    /**
     * @brief Wait for a process with the specified name to appear.
     *
     * The function periodically queries the operating system until either
     * the process is found or the timeout expires.
     *
     * @param queryName      Executable name of the process.
     * @param timeout        Maximum time to wait.
     * @param pollInterval   Delay between lookup attempts.
     *
     * @return ProcessInfo if the process was found, otherwise std::nullopt.
     *
     * If the timeout expires, the error code 'OperationTimeout' is set.
     */
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
                EProcessArchitecture arch = EProcessArchitecture::Unknown;
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

    /**
     * @brief Retrieve the process ID of the first process with the given name.
     *
     * @return Process ID, or 0 if no process was found.
     */
    uint32_t getProcessID(const std::string &name) const
    {
        uint32_t procID = 0;
        _platformLink->getProcessIDByName(name, procID);
        return procID;
    }

    /**
     * @brief Retrieve all process IDs matching the given name.
     */
    std::vector<uint32_t> getProcessIDs(const std::string &name) const
    {
        std::vector<uint32_t> procIDs = {};
        _platformLink->getProcessIDsByName(name, procIDs);
        return procIDs;
    }

    /**
     * @brief Retrieve the process ID owning a window with the given title.
     *
     * @return Process ID, or 0 if no matching window was found.
     */
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