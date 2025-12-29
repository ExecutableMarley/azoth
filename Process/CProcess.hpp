#pragma once

#include <string>

#include "Types/Address.hpp"
#include "Types/EProcessArchitecture.hpp"
#include "Types/EProcessPrivilegeLevel.hpp"
#include "Platform/IPlatformLink.hpp"
#include "ProcessModules/CMemoryModule.hpp"
#include "ProcessModules/CDecoderModule.hpp"
#include "ProcessModules/CScannerModule.hpp"

namespace Azoth
{


class CProcess
{
	friend class CMemoryModule;
	friend class CDecoderModule;
	friend class CScannerModule;
public:
	CProcess(IPlatformLink* platformLink) : _platformLink(platformLink),
		_memory(this, platformLink), _decoder(this), _scanner(this),
		_processID(0)
	{
		this->_name = std::string();
		this->_path = std::string();
		this->_architecture   = EProcessArchitecture::Unknown;
		this->_privilegeLevel = EProcessPrivilegeLevel::Unknown;
	}

public:

	//=== Essential ===//

	bool initialize()
	{
		return _platformLink->initialize();
	}

	bool isInitialized() const
	{
		return _platformLink->isInitialized();
	}

	bool attach(uint32_t processID)
	{
		if (_platformLink->attach(processID))
		{
			this->_processID = processID;
			return true;
		}
		return false;
	}

	bool isAttached() const
	{
		return _platformLink->isAttached();
	}

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

	//Process flow
	
	bool isAlive()
	{
		return _platformLink->isAlive();
	}

	bool terminate()
	{
		return _platformLink->terminate();
	}

	bool suspend()
	{
		return _platformLink->suspend();
	}

	bool resume()
	{
		return _platformLink->resume();
	}

	//RestoreAll?


	//[Process Images]

	ProcessImage getProcessMainImage()
	{
		return _platformLink->getMainProcessImage();
	}

	ProcessImage getProcessImage(const std::string& name)
	{
		return _platformLink->getProcessImage(name);
	}

	std::vector<ProcessImage> getAllProcessImages()
	{
		return _platformLink->getAllProcessImages();
	}

	//ImageSymbols?

	//Threads

private:
	IPlatformLink* _platformLink;
	uint32_t _processID;
	std::string _name;
	std::string _path;
	EProcessArchitecture   _architecture;
	EProcessPrivilegeLevel _privilegeLevel;

public:
	CMemoryModule& getMemory() { return _memory; }
	CDecoderModule& getDecoder() { return _decoder; }
	CScannerModule& getScanner() { return _scanner; }

private:
	CMemoryModule  _memory;
	CDecoderModule _decoder;
	CScannerModule _scanner;
};


}