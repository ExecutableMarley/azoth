/*
 * MIT License
 *
 * Copyright (c) Marley Arns
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "Types/Address.hpp"
#include "Types/EProcessArchitecture.hpp"
#include "Types/EProcessPrivilegeLevel.hpp"
#include "Platform/IPlatformLink.hpp"
#include "ProcessModules/CMemoryModule.hpp"
#include "ProcessModules/CSymbolModule.hpp"
#include "ProcessModules/CDecoderModule.hpp"
#include "ProcessModules/CScannerModule.hpp"
#include "CSystem.hpp"

#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <cassert>

namespace Azoth
{


/**
 * @brief High-level abstraction of an operating system process.
 *
 * CProcess represents a single target process and acts as the central
 * coordination point for all process-related functionality.
 *
 * All platform-specific behavior is delegated to the provided
 * IPlatformLink implementation.
 *
 * A CProcess instance follows this lifecycle:
 *   1. initialize()
 *   2. attach(processID)
 *   3. Perform operations (memory, scanning, control, etc.)
 *   4. detach()
 */
class CProcess
{
	friend class CMemoryModule;
	friend class CDecoderModule;
	friend class CScannerModule;
public:
	/**
	 * @brief Construct a process wrapper using a platform backend.
	 *
	 * @param platformLink Platform-specific implementation. 
	 * Ownership of the backend is transferred to the CSystem instance.
	 *
	 * @note The CProcess object does not automatically initialize or attach.
	 * These steps must be performed explicitly.
	 */
	CProcess(std::unique_ptr<IPlatformLink> platformLink) : _platformLink(std::move(platformLink)),
		_memory(this, _platformLink.get()), _symbols(this, _platformLink.get()), _decoder(this), _scanner(this),
		_processID(0)
	{
		assert(_platformLink);
		this->_name = std::string();
		this->_path = std::string();
		this->_architecture   = EProcessArchitecture::Unknown;
		this->_privilegeLevel = EProcessPrivilegeLevel::Unknown;
	}

	CProcess(const CProcess&) = delete;

	CProcess& operator=(const CProcess&) = delete;

public:

	//=== Initialization & attachment ===//

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

	/**
	 * @brief Attach to a target process by process ID.
	 *
	 * @param processID Target process ID.
	 * @return True if the process was successfully attached.
	 */
	bool attach(uint32_t processID)
	{
		if (_platformLink->attach(processID))
		{
			this->_processID = processID;
			this->retrieveProcessData();
			this->_decoder.setTargetArchitecture(_architecture);
			return true;
		}
		return false;
	}

	/**
	 * @brief Check whether a process is currently attached.
	 */
	bool isAttached() const
	{
		return _platformLink->isAttached();
	}

	/**
	 * @brief Detach from the currently attached process.
	 *
	 * After detaching, the CProcess instance returns to an unattached state
	 * and can be attached again.
	 */

	void detach()
	{
		_processID = 0;
		_platformLink->detach();
	}

	//=== Process information ===//

	uint32_t GetProcessID()      const { return _processID; }
	const std::string& GetName() const { return _name; }
	const std::string& GetPath() const { return _path; }
	EProcessArchitecture GetArchitecture()     const { return _architecture; }
	EProcessPrivilegeLevel GetPrivilegeLevel() const { return _privilegeLevel; }
private:
	void retrieveProcessData()
	{
		_platformLink->getProcessName(_processID, _name);
		_platformLink->getProcessPath(_processID, _path);
		_platformLink->getProcessArchitecture(_processID, _architecture);
		_platformLink->getProcessPrivilegeLevel(_processID, _privilegeLevel);
	}
public:

	//Process flow
	
	/**
	 * @brief Check whether the attached process is still alive.
	 */
	bool isAlive()
	{
		bool isAliveState = false;
		_platformLink->isAlive(isAliveState);
		return isAliveState;
	}

	/**
	 * @brief Terminate the attached process.
	 */
	bool terminate()
	{
		return _platformLink->terminate();
	}

	/**
	 * @brief Suspend execution of the attached process.
	 */
	bool suspend()
	{
		return _platformLink->suspend();
	}

	/**
	 * @brief Resume execution of the attached process.
	 */
	bool resume()
	{
		return _platformLink->resume();
	}

	//RestoreAll?


	//[Process Images]

	/**
	 * @brief Retrieve the main executable image of the process.
	 */
	ProcessImage getProcessMainImage()
	{
		ProcessImage procImage;
		_platformLink->getMainProcessImage(procImage);
		return procImage;
	}

	/**
	 * @brief Retrieve a loaded process image by name.
	 *
	 * @param name Module or image name.
	 */
	ProcessImage getProcessImage(const std::string& name)
	{
		ProcessImage procImage;
		_platformLink->getProcessImage(name, procImage);
		return procImage;
	}

	/**
	 * @brief Retrieve all loaded images of the process.
	 */
	std::vector<ProcessImage> getAllProcessImages()
	{
		std::vector<ProcessImage> procImages;
		_platformLink->getAllProcessImages(procImages);
		return procImages;
	}

	/**
 	 * @brief Attempts to retrieve a process image, polling if not immediately found.
	 * 
	 * @param name      Module or image name.
	 * @param outImage  Reference to store the retrieved image.
	 * @param timeout   Maximum time to wait (e.g., std::chrono::milliseconds).
	 * @param interval  Time between retries.
	 * @return          true if found within the timeout, false otherwise.
	 */
	bool waitForProcessImage(const std::string& name, ProcessImage& outImage, 
                         std::chrono::milliseconds timeout = std::chrono::milliseconds(2000), 
                         std::chrono::milliseconds interval = std::chrono::milliseconds(100)) 
	{
    	auto start = std::chrono::steady_clock::now();
    	while (std::chrono::steady_clock::now() - start < timeout)
		{
        	if (_platformLink->getProcessImage(name, outImage))
			{
        	    if (outImage.valid())
				{ 
        	    	return true; 
        	    }
        	}
        	std::this_thread::sleep_for(interval);
    	}
    	return _platformLink->setError(EPlatformError::OperationTimeout);
	}

	//Threads

private:
	std::unique_ptr<IPlatformLink> _platformLink;
	uint32_t _processID;
	std::string _name;
	std::string _path;
	EProcessArchitecture   _architecture;
	EProcessPrivilegeLevel _privilegeLevel;

	//std::vector<ProcessImage> _processImageCache;
	//std::vector<std::vector<ImageSymbol>> _symbols;

public:
	CMemoryModule& getMemory() { return _memory; }
	CSymbolModule& getSymbols() { return _symbols; }
	CDecoderModule& getDecoder() { return _decoder; }
	CScannerModule& getScanner() { return _scanner; }

private:
	CMemoryModule  _memory;
	CSymbolModule  _symbols;
	CDecoderModule _decoder;
	CScannerModule _scanner;
};


}