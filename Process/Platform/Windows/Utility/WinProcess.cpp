#include "WinProcess.hpp"

#include <vector>
#include <iostream>

#include "../../../Core/ProcessImage.hpp"

#undef UNICODE
#undef _UNICODE

#include <Windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <TlHelp32.h>
#include <Wow64apiset.h>

#include "SmartHandle.hpp"
#include "WStringUtil.hpp"
#include "WinTypes.hpp"

namespace Azoth
{


PlatformErrorState retrieveProcessIDByName(const std::string& procName, uint32_t& out_process_id)
{
	SmartHandle toolSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (!toolSnapshot.isValid())
		return { EPlatformError::InternalError, GetLastError() };

	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	pEntry.th32ProcessID = 0;
	if (!Process32First(toolSnapshot, &pEntry))
		return { EPlatformError::InternalError, GetLastError() };
	do
	{
		//Performs an ordinal Comparison, returns false (0) if equal
		if (!strcmp(pEntry.szExeFile, procName.c_str()))
		{
			out_process_id = pEntry.th32ProcessID;
			return { EPlatformError::Success, 0};
		}
	} while (Process32Next(toolSnapshot, &pEntry));
	return { EPlatformError::ResourceNotFound, 0};
}

PlatformErrorState retrieveProcessIDByName(const std::string& procName, std::vector<uint32_t>& out_process_ids)
{
	out_process_ids.clear();
	SmartHandle toolSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	pEntry.th32ProcessID = 0;
	if (!Process32First(toolSnapshot, &pEntry))
	{
		return { EPlatformError::InternalError, GetLastError() };
	}
	do
	{
		//Performs an ordinal Comparison, returns false (0) if equal
		if (!strcmp(pEntry.szExeFile, procName.c_str()))
		{
			out_process_ids.push_back(pEntry.th32ProcessID);
		}
	} while (Process32Next(toolSnapshot, &pEntry));

	if (out_process_ids.empty())
		return { EPlatformError::ResourceNotFound, 0 };
	else
		return { EPlatformError::Success, 0};
}

// NtQuerySystemInformation
using fnNtQuerySystemInformation = decltype(&NtQuerySystemInformation);

bool retrieveProcessIDByNameFallback(const std::string& process_name, uint32_t& out_process_id)
{
	out_process_id = 0;

	std::wstring wprocess_name = StringUtil::toUtf16(process_name);
	std::wstring wprocess_name_lower = StringUtil::to_lower(wprocess_name);

	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (!ntdll) return false;

	fnNtQuerySystemInformation NtQuerySystemInformation =
		(fnNtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");

	if (!NtQuerySystemInformation) return false;

	ULONG size = 0x10000; // (64KB)
	NTSTATUS status;
	std::vector<uint8_t> system_information_buffer;

	do
	{
		system_information_buffer.resize(size);
		PVOID buffer = system_information_buffer.data();

		// 'size' will be updated with the required size on failure
		status = NtQuerySystemInformation(
			SystemProcessInformation,
			buffer,
			size,
			&size
		);

		// 0xC0000004 or 0xC0000008
		if (status == STATUS_INFO_LENGTH_MISMATCH || status == 0xC0000008)
		{
			if (size > 0x1000000) return false; // Safety (16MB)
			continue;
		}

		break;

	} while (true);

	if (!NT_SUCCESS(status))
	{
		return false; // Unrecoverable
	}

	SYSTEM_PROCESS_INFORMATION* current_info = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(system_information_buffer.data());

	while (true)
	{
		// Check if name is avaiable
		if (current_info->ImageName.Buffer)
		{
			std::wstring current_image_name(current_info->ImageName.Buffer, current_info->ImageName.Length / sizeof(wchar_t));
			std::wstring current_image_name_lower = StringUtil::to_lower(current_image_name);

			if (current_image_name_lower == wprocess_name_lower)
			{
				out_process_id = (uint64_t)(ULONG_PTR)current_info->UniqueProcessId;
				return true;
			}
		}

		// Check if this is the last entry
		if (current_info->NextEntryOffset == 0)
		{
			break;
		}

		// Advance to the next entry
		current_info = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(
			reinterpret_cast<uint8_t*>(current_info) + current_info->NextEntryOffset
			);
	}

	// In this case the process simply did not exist
	return true;
}

bool retrieveProcessIDByNameFallback(const std::string& process_name, std::vector<uint32_t>& process_ids)
{
	process_ids.clear();

	std::wstring wprocess_name = StringUtil::toUtf16(process_name);
	std::wstring wprocess_name_lower = StringUtil::to_lower(wprocess_name);

	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (!ntdll) return false;

	fnNtQuerySystemInformation NtQuerySystemInformation =
		(fnNtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");

	if (!NtQuerySystemInformation) return false;

	ULONG size = 0x10000; // (64KB)
	NTSTATUS status;
	std::vector<uint8_t> system_information_buffer;

	do
	{
		system_information_buffer.resize(size);
		PVOID buffer = system_information_buffer.data();

		// 'size' will be updated with the required size on failure
		status = NtQuerySystemInformation(
			SystemProcessInformation,
			buffer,
			size,
			&size
		);

		// 0xC0000004 or 0xC0000008
		if (status == STATUS_INFO_LENGTH_MISMATCH || status == 0xC0000008)
		{
			if (size > 0x1000000) return false; // Safety (16MB)
			continue;
		}

		break;

	} while (true);

	if (!NT_SUCCESS(status))
	{
		return false; // Unrecoverable
	}

	SYSTEM_PROCESS_INFORMATION* current_info = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(system_information_buffer.data());

	while (true)
	{
		// Check if name is avaiable
		if (current_info->ImageName.Buffer)
		{
			std::wstring current_image_name(current_info->ImageName.Buffer, current_info->ImageName.Length / sizeof(wchar_t));
			std::wstring current_image_name_lower = StringUtil::to_lower(current_image_name);

			if (current_image_name_lower == wprocess_name_lower)
			{
				process_ids.push_back((uint64_t)(ULONG_PTR)current_info->UniqueProcessId);
				//out_process_id = (uint64_t)(ULONG_PTR)current_info->UniqueProcessId;
				return true;
			}
		}

		// Check if this is the last entry
		if (current_info->NextEntryOffset == 0)
		{
			break;
		}

		// Advance to the next entry
		current_info = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(
			reinterpret_cast<uint8_t*>(current_info) + current_info->NextEntryOffset
			);
	}

	// In this case the process simply did not exist
	return true;
}

PlatformErrorState retrieveProcessIDByWindowName(const std::string& windowName, uint32_t& out_process_id)
{
	HWND hwnd = FindWindowA(NULL, windowName.c_str());

	if (hwnd == NULL)
	{
		return { EPlatformError::InternalError, GetLastError() };
	}

	DWORD processID = 0;
    if (GetWindowThreadProcessId(hwnd, &processID) == 0)
	{
		return { EPlatformError::InternalError, GetLastError() };
	}

	if (processID == 0)
	{
		return { EPlatformError::ResourceNotFound, 0 };
	}

	out_process_id = static_cast<uint32_t>(processID);
	return { EPlatformError::Success, 0 };
}

//

PlatformErrorState retrieveProcessMainImage(uint32_t pid, ProcessImage& procImage)
{
	SmartHandle hSnap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
	if (!hSnap.isValid())
        return { EPlatformError::InternalError, GetLastError() };
	
	MODULEENTRY32 mEntry;
	mEntry.dwSize = sizeof(mEntry);

	if (Module32First(hSnap, &mEntry))
	{
		procImage = fromWinModule(mEntry);
		return { EPlatformError::Success, 0 };
	}
	
	DWORD errorCode = GetLastError();
	if (errorCode == ERROR_NO_MORE_FILES)
	{
		return { EPlatformError::ResourceNotFound, 0 };
	}
	else
	{
		return { EPlatformError::InternalError, GetLastError() };
	}
}

PlatformErrorState retrieveProcessImage(uint32_t pid, const std::string& imageName, ProcessImage& procImage)
{
	SmartHandle hSnap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
	if (!hSnap.isValid())
        return { EPlatformError::InternalError, GetLastError() };
	
	MODULEENTRY32 mEntry;
	mEntry.dwSize = sizeof(mEntry);
    
	if (Module32First(hSnap, &mEntry))
	{
		do
		{
			if (!strcmp(mEntry.szModule, imageName.c_str()))
			{
				procImage = fromWinModule(mEntry);
				return { EPlatformError::Success, 0 };
			}
		} while (Module32Next(hSnap, &mEntry));
	}

	DWORD errorCode = GetLastError();
	if (errorCode == ERROR_NO_MORE_FILES)
	{
		return { EPlatformError::ResourceNotFound, 0 };
	}
	else
	{
		return { EPlatformError::InternalError, GetLastError() };
	}
}

PlatformErrorState retrieveAllProcessImages(uint32_t pid, std::vector<ProcessImage>& procImages)
{
	procImages.clear();
	SmartHandle hSnap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
	if (!hSnap.isValid())
        return { EPlatformError::InternalError, GetLastError() };
	
	MODULEENTRY32 moduleEntry;
	moduleEntry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(hSnap, &moduleEntry))
	{
		do
		{
			procImages.push_back(fromWinModule(moduleEntry));
		} while (Module32Next(hSnap, &moduleEntry));
	}

	DWORD errorCode = GetLastError();
	if (errorCode == ERROR_NO_MORE_FILES)
	{
		if (procImages.empty())
			return { EPlatformError::ResourceNotFound, 0 };
		else
			return { EPlatformError::Success, 0 };
	}
	else
	{
		return { EPlatformError::InternalError, GetLastError() };
	}
}

//

PlatformErrorState retrieveProcessPrivilegeLevel(HANDLE processHandle, EProcessPrivilegeLevel& outLevel)
{
    outLevel = EProcessPrivilegeLevel::Unknown;

    if (!processHandle)
        return { EPlatformError::InvalidArgument, 0 };

    HANDLE token = nullptr;
    if (!OpenProcessToken(processHandle, TOKEN_QUERY, &token))
        return { EPlatformError::InternalError, 0 };

    TOKEN_ELEVATION elevation{};
    DWORD size = 0;

	// Check for user elevation
    if (!GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
    {
        DWORD err = GetLastError();
        CloseHandle(token);
        return { EPlatformError::InternalError, err };
    }

    outLevel = elevation.TokenIsElevated
        ? EProcessPrivilegeLevel::ElevatedUser
        : EProcessPrivilegeLevel::StandardUser;

    // Check for SYSTEM explicitly
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID systemSid = nullptr;

    if (AllocateAndInitializeSid(
            &ntAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0,0,0,0,0,0,0,
            &systemSid))
    {
        BOOL isSystem = FALSE;
        if (CheckTokenMembership(token, systemSid, &isSystem) && isSystem)
            outLevel = EProcessPrivilegeLevel::System;

        FreeSid(systemSid);
    }

    CloseHandle(token);
	return { EPlatformError::Success, 0 };
}

PlatformErrorState retrieveProcessArchitecture(HANDLE processHandle, EProcessArchitecture& outArch)
{
    outArch = EProcessArchitecture::Unknown;

    if (!processHandle)
        return { EPlatformError::InvalidArgument, 0};

	typedef BOOL (WINAPI *PISWOW64PROCESS2)(HANDLE, USHORT*, USHORT*);

    // Attempt IsWow64Process2 (Win10+)
    static auto pIsWow64Process2 =
        reinterpret_cast<PISWOW64PROCESS2>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process2"));

    if (pIsWow64Process2)
    {
        USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
        USHORT nativeMachine  = IMAGE_FILE_MACHINE_UNKNOWN;

        if (pIsWow64Process2(processHandle, &processMachine, &nativeMachine))
        {
            USHORT machine = (processMachine != IMAGE_FILE_MACHINE_UNKNOWN)
                                 ? processMachine
                                 : nativeMachine;

            switch (machine)
            {
            case IMAGE_FILE_MACHINE_I386:  outArch = EProcessArchitecture::x86;   break;
            case IMAGE_FILE_MACHINE_AMD64: outArch = EProcessArchitecture::x64;   break;
            case IMAGE_FILE_MACHINE_ARM:   outArch = EProcessArchitecture::ARM32; break;
            case IMAGE_FILE_MACHINE_ARM64: outArch = EProcessArchitecture::ARM64; break;
            default: return { EPlatformError::NotSupported, machine }; //Todo: Better Error code
            }

            return { EPlatformError::Success, 0 };
        }
    }

    // Fallback: IsWow64Process
    BOOL isWow64 = FALSE;
    if (!IsWow64Process(processHandle, &isWow64))
        return { EPlatformError::InternalError, GetLastError() };;

#if defined(_WIN64)
    outArch = isWow64 ? EProcessArchitecture::x86 : EProcessArchitecture::x64;
#else
    outArch = EProcessArchitecture::x86;
#endif

    return { EPlatformError::Success, 0 };
}

PlatformErrorState retrieveProcessName(HANDLE processHandle, std::string& outProcessName)
{
    outProcessName.clear();

    if (!processHandle)
        return { EPlatformError::InvalidArgument, 0};;

    char buffer[MAX_PATH];
    DWORD size = MAX_PATH;

    if (!QueryFullProcessImageNameA(processHandle, 0, buffer, &size))
        return { EPlatformError::InternalError, GetLastError() };

    const char* name = strrchr(buffer, '\\');
    outProcessName = name ? name + 1 : buffer;
    return { EPlatformError::Success, 0 };
}

PlatformErrorState retrieveProcessPath(HANDLE processHandle, std::string& outProcessPath)
{
    outProcessPath.clear();

    if (!processHandle)
        return { EPlatformError::InvalidArgument, 0};

    char buffer[MAX_PATH];
    DWORD size = MAX_PATH;

    if (!QueryFullProcessImageNameA(processHandle, 0, buffer, &size))
        return { EPlatformError::InternalError, GetLastError() };

    outProcessPath.assign(buffer, size);
    return { EPlatformError::Success, 0 };
}


}