#pragma once

#include "Utility/WinTypes.hpp"
#include "../IPlatformLink.hpp"

#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <tlhelp32.h>

#include <exception>

namespace Azoth
{


//Todo: Consider moving logic to helper files

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
		HANDLE tmpHandle = OpenProcess(PROCESS_ALL_ACCESS, false, procID);

		if (tmpHandle == INVALID_HANDLE_VALUE)
			return false;

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

	bool isAlive() const override
	{
		return this->getExitCode() == STILL_ACTIVE;
	}

	uint32_t getExitCode() const override
	{ 
		DWORD exitCode;
		if (GetExitCodeProcess(this->_hProcess, &exitCode))
			return exitCode;
		return 0;
	}

	//=== Process Images ===//

	ProcessImage getMainProcessImage() const override
	{
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->_procID);
		if (MODULEENTRY32 mEntry; Module32First(hSnap, &mEntry))
		{
			CloseHandle(hSnap);
			return fromWinModule(mEntry);
		}
		CloseHandle(hSnap);
		return ProcessImage();
	}

	ProcessImage getProcessImage(const std::string& name) const override
	{
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->_procID);
		MODULEENTRY32 mEntry;
		mEntry.dwSize = sizeof(mEntry);
        
		if (Module32First(hSnap, &mEntry))
		{
			do
			{
				if (!strcmp(mEntry.szModule, name.c_str()))
				{
					CloseHandle(hSnap);
					return fromWinModule(mEntry);
				}
			} while (Module32Next(hSnap, &mEntry));
		}
		CloseHandle(hSnap);
		return ProcessImage();
	}

	std::vector<ProcessImage> getAllProcessImages() const override
	{
		std::vector<ProcessImage> moduleList;

		HANDLE hModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->_procID);
		MODULEENTRY32 moduleEntry;
		moduleEntry.dwSize = sizeof(tagMODULEENTRY32);

		if (Module32First(hModule, &moduleEntry))
		{
			do
			{
				moduleList.push_back(fromWinModule(moduleEntry));
			} while (Module32Next(hModule, &moduleEntry));
		}
		CloseHandle(hModule);
		return moduleList;
	}

	//=== Process Query ===//

	uint32_t getProcessIDByName(const std::string& name) const override
	{
		HANDLE toolSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 pEntry;
		pEntry.dwSize = sizeof(pEntry);
		pEntry.th32ProcessID = 0;

		if (!Process32First(toolSnapshot, &pEntry))
			return 0;
		do
		{
			//Performs an ordinal Comparison, returns false (0) if equal
			if (!strcmp(pEntry.szExeFile, name.c_str()))
			{
				CloseHandle(toolSnapshot);
				return pEntry.th32ProcessID;
			}
		} while (Process32Next(toolSnapshot, &pEntry));
		return 0;
	}

	std::vector<uint32_t> getProcessIDsByName(const std::string& name) const override
	{
		HANDLE toolSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 pEntry;
		pEntry.dwSize = sizeof(pEntry);
		pEntry.th32ProcessID = 0;
		std::vector<uint32_t> ids;

		if (!Process32First(toolSnapshot, &pEntry))
		{
			CloseHandle(toolSnapshot);
			return ids;
		}
		do
		{
			//Performs an ordinal Comparison, returns false (0) if equal
			if (!strcmp(pEntry.szExeFile, name.c_str()))
			{
				ids.push_back(pEntry.th32ProcessID);
			}
		} while (Process32Next(toolSnapshot, &pEntry));
		CloseHandle(toolSnapshot);
		return ids;
	}

	uint32_t getProcessIDByWindowName(const std::string& windowTitle) const override
	{
		return 0;
	}

	std::vector<uint32_t> getProcessIDsByWindowName(const std::string& windowTitle) const override
	{
		return {};
	}

	//=== Memory ===//

	bool read(uint64_t addr, size_t size, void* buffer) const override
	{
		size_t bytesRead;
		if (ReadProcessMemory(this->_hProcess, (LPVOID)addr, buffer, size, (SIZE_T*)&bytesRead) && bytesRead != 0)
			return true;

		return false;
	}

	bool write(uint64_t addr, size_t size, const void* buffer) override
	{
		size_t bytesRead;
		if (WriteProcessMemory(this->_hProcess, (LPVOID)addr, buffer, size, (SIZE_T*)&bytesRead) && bytesRead != 0)
			return true;

		return false;
	}

	bool queryMemory(uint64_t addr, MemoryRegion& memoryRegion) const override
	{
		MEMORY_BASIC_INFORMATION mbi{};
		if (VirtualQueryEx(this->_hProcess, (LPVOID)addr, &mbi, sizeof(mbi)) == 0)
		{
			return false;
		}

		memoryRegion = fromWinBasicInformation(mbi);
		return true;
	}

	bool virtualProtect(uint64_t addr, size_t size, EMemoryProtection newProtect, EMemoryProtection* oldProtect) override
	{
		DWORD winNewProtect = toWinProtect(newProtect);
		DWORD winOldProtection = 0;
		if (VirtualProtectEx(this->_hProcess, (LPVOID)addr, size, winNewProtect, (PDWORD)(&winOldProtection)))
		{
			if (oldProtect)
				*oldProtect = fromWinProtect(winOldProtection);
			return true;
		}
		return false;
	}

	uint64_t virtualAllocate(uint64_t addr, size_t size, EMemoryProtection protection) override
	{
		auto winProt = toWinProtect(protection);
		addr = (uint64_t)VirtualAllocEx(this->_hProcess, NULL, size,
			MEM_COMMIT | MEM_RESERVE, winProt);
		return addr;
	}

	bool virtualFree(uint64_t addr) override
	{
		return VirtualFreeEx(this->_hProcess, (LPVOID)addr, 0, MEM_RELEASE);
	}

	//=== Threads ===//


private:
	uint32_t _procID;
	HANDLE _hProcess;
};


}