/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#ifdef _WIN32

#include "../IPlatformLink.hpp"

#include "Utility/WinTypes.hpp"
#include "Utility/WinProcess.hpp"

#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <ntstatus.h>
#include <winternl.h>

#include <exception>

namespace Azoth
{


//Todo: Consider moving logic to helper files

/**
 * @internal
 * @brief Default Windows Implementation
 */
class WinapiLink : public IPlatformLink
{
public:
	WinapiLink() : _procID(0), _hProcess(INVALID_HANDLE_VALUE)
	{

	}

	~WinapiLink()
	{
		detach();
	}

	//=== Generic ===//

	bool initialize() override
	{
		return true;
	}

	bool isInitialized() const override
	{
		return true;
	}

	bool attach(uint32_t procID) override
	{
		if (isAttached())
            return setError(EPlatformError::InvalidState);

		HANDLE tmpHandle = OpenProcess(PROCESS_ALL_ACCESS, false, procID);

		if (tmpHandle == INVALID_HANDLE_VALUE)
			return setError(EPlatformError::InternalError, GetLastError());

		this->_procID = procID;
		this->_hProcess = tmpHandle;

		return true;
	}

	bool isAttached() const override
	{
		return this->_procID != 0;
	}

	void detach() override
	{
		_procID = 0;
		CloseHandle(_hProcess);
		_hProcess = INVALID_HANDLE_VALUE;
	}

	//=== Process Specific ===//

	bool isAlive(bool& isAliveState) const override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		uint32_t exitCode;
		if (this->getExitCode(exitCode))
		{
			isAliveState = exitCode == STILL_ACTIVE;
			return setError(EPlatformError::Success);
		}
		return false;
	}

	bool terminate()
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		DWORD exitCode = 1;
		if (TerminateProcess(this->_hProcess, exitCode))
		{
			return true;
		}
		else
		{
			return setError(EPlatformError::InternalError, GetLastError());
		}
	}

	bool suspend()
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		typedef NTSTATUS( NTAPI* fnNtSuspendProcess )(HANDLE ProcessHandle);
		static fnNtSuspendProcess suspendProcess = nullptr;

		if (!suspendProcess) 
		{
    		auto hNtdll = GetModuleHandleW(L"ntdll.dll");
    		if (!hNtdll)
        		return setError(EPlatformError::SymbolNotFound);
			
			suspendProcess = reinterpret_cast<fnNtSuspendProcess>(GetProcAddress(hNtdll, "NtSuspendProcess"));
		}
		if (!suspendProcess)
        	return setError(EPlatformError::SymbolNotFound);
		
		NTSTATUS status = suspendProcess(this->_hProcess);
		if (!NT_SUCCESS(status))
		{
			return setError(EPlatformError::InternalError, status);
		}
		return true;
	}

	bool resume()
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		typedef NTSTATUS( NTAPI* fnNtResumeProcess )(HANDLE ProcessHandle);
		static fnNtResumeProcess resumeProcess = nullptr;

		if (!resumeProcess)
		{
        	auto hNtdll = GetModuleHandleW(L"ntdll.dll");
        	if (!hNtdll)
            	return setError(EPlatformError::SymbolNotFound);

        	resumeProcess = reinterpret_cast<fnNtResumeProcess>(GetProcAddress(hNtdll, "NtResumeProcess"));
    	}
		if (!resumeProcess)
            return setError(EPlatformError::SymbolNotFound);

		NTSTATUS status = resumeProcess(this->_hProcess);
		if (!NT_SUCCESS(status))
		{
			return setError(EPlatformError::InternalError, status);
		}
		return true;
	}

	bool getExitCode(uint32_t& exitCode) const override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		DWORD eCode;
		if (GetExitCodeProcess(this->_hProcess, &eCode))
		{
			exitCode = eCode;
			return setError(EPlatformError::Success);
		}
		return setError(EPlatformError::InternalError, GetLastError());
	}

	//=== Process Images ===//

	bool getMainProcessImage(ProcessImage& processImage) const override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		return setError(retrieveProcessMainImage(this->_procID, processImage));
	}

	bool getProcessImage(const std::string& name, ProcessImage& processImage) const override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		return setError(retrieveProcessImage(this->_procID, name, processImage));
	}

	bool getAllProcessImages(std::vector<ProcessImage>& processImages) const override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		return setError(retrieveAllProcessImages(this->_procID, processImages));
	}

	bool getExportSymbols(const ProcessImage& image, std::vector<ImageSymbol>& symbols) const override
	{
		if (!isAttached())
			return setError(EPlatformError::InvalidState);

		return setError(retrieveExportSymbols(image, symbols));
	}

	bool getImportSymbols(const ProcessImage& image, std::vector<ImageSymbol>& symbols) const override
	{
		if (!isAttached())
			return setError(EPlatformError::InvalidState);

		return setError(retrieveImportSymbols(image, symbols));
	}

	//=== Process Query ===//

	bool getProcessIDByName(const std::string& name, uint32_t& procID) const override
	{
		return setError(retrieveProcessIDByName(name, procID));
	}

	bool getProcessIDsByName(const std::string& name, std::vector<uint32_t>& procIDs) const override
	{
		return setError(retrieveProcessIDByName(name, procIDs));
	}

	bool getProcessIDByWindowName(const std::string& windowTitle, uint32_t& procID) const override
	{
		return setError(retrieveProcessIDByWindowName(windowTitle, procID));
	}

	bool getProcessName(uint32_t procID, std::string& name) const override
	{
		if (procID == this->_procID)
			return setError(retrieveProcessName(_hProcess, name));
		else
			return setError(retrieveProcessName(procID, name));
	}

	bool getProcessPath(uint32_t procID, std::string& path) const override
	{
		if (procID == this->_procID)
			return setError(retrieveProcessPath(_hProcess, path));
		else
			return setError(retrieveProcessName(procID, path));
	}

	bool getProcessArchitecture(uint32_t procID, EProcessArchitecture& architecture) const override
	{
		if (procID == this->_procID)
			return setError(retrieveProcessArchitecture(_hProcess, architecture));
		else
			return setError(retrieveProcessArchitecture(procID, architecture));
	}

	bool getProcessPrivilegeLevel(uint32_t procID, EProcessPrivilegeLevel& privileges) const override
	{
		if (procID == this->_procID)
			return setError(retrieveProcessPrivilegeLevel(_hProcess, privileges));
		else
			return setError(retrieveProcessPrivilegeLevel(procID, privileges));
	}

	//=== Memory ===//

	bool read(uint64_t addr, size_t size, void* buffer) const override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		size_t bytesRead;
		if (ReadProcessMemory(this->_hProcess, (LPVOID)addr, buffer, size, (SIZE_T*)&bytesRead) && bytesRead != 0)
			return true;

		return setError(EPlatformError::InternalError, GetLastError());
	}

	bool write(uint64_t addr, size_t size, const void* buffer) override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		size_t bytesRead;
		if (WriteProcessMemory(this->_hProcess, (LPVOID)addr, buffer, size, (SIZE_T*)&bytesRead) && bytesRead != 0)
			return true;

		return setError(EPlatformError::InternalError, GetLastError());
	}

	bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion) const override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		MEMORY_BASIC_INFORMATION mbi{};
		if (VirtualQueryEx(this->_hProcess, (LPVOID)addr, &mbi, sizeof(mbi)) != 0)
		{
			memoryRegion = fromWinBasicInformation(mbi);
			return true;
		}
		return setError(EPlatformError::InternalError, GetLastError());
	}

	bool virtualProtect(uint64_t addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* oldProtect) override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		DWORD winNewProtect = toWinProtect(newProtect);
		DWORD winOldProtection = 0;
		if (VirtualProtectEx(this->_hProcess, (LPVOID)addr, size, winNewProtect, (PDWORD)(&winOldProtection)))
		{
			if (oldProtect)
				*oldProtect = fromWinProtect(winOldProtection);
			return true;
		}
		return setError(EPlatformError::InternalError, GetLastError());
	}

	uint64_t virtualAllocate(uint64_t addr, size_t size, EMemoryProtection protection) override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		auto winProt = toWinProtect(protection);
		addr = (uint64_t)VirtualAllocEx(this->_hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, winProt);
		
		if (addr)
			return addr;

		return (uint64_t)setError(EPlatformError::InternalError, GetLastError());
	}

	bool virtualFree(uint64_t addr) override
	{
		if (!isAttached())
            return setError(EPlatformError::InvalidState);

		if (VirtualFreeEx(this->_hProcess, (LPVOID)addr, 0, MEM_RELEASE))
		{
			return true;
		}
		return setError(EPlatformError::InternalError, GetLastError());
	}

	//=== Threads ===//


protected:
	uint32_t _procID;
	HANDLE _hProcess;
};


}


#endif