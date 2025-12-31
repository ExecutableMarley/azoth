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

#include "WStringUtil.hpp"

namespace Azoth
{


bool retrieveProcessIDByName(const std::string& procName, std::uint32_t& out_process_id)
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
		if (!strcmp(pEntry.szExeFile, procName.c_str()))
		{
			out_process_id = pEntry.th32ProcessID;
			break;
		}
	} while (Process32Next(toolSnapshot, &pEntry));
	CloseHandle(toolSnapshot);
	return false;
}

bool retrieveProcessIDByName(const std::string& procName, std::vector<std::uint32_t>& out_process_ids)
{
	out_process_ids.clear();
	HANDLE toolSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	pEntry.th32ProcessID = 0;
	if (!Process32First(toolSnapshot, &pEntry))
	{
		CloseHandle(toolSnapshot);
		return false;
	}
	do
	{
		//Performs an ordinal Comparison, returns false (0) if equal
		if (!strcmp(pEntry.szExeFile, procName.c_str()))
		{
			out_process_ids.push_back(pEntry.th32ProcessID);
		}
	} while (Process32Next(toolSnapshot, &pEntry));
	CloseHandle(toolSnapshot);
	return true;
}

// NtQuerySystemInformation
using fnNtQuerySystemInformation = decltype(&NtQuerySystemInformation);

bool retrieveProcessIDByNameFallback(const std::string& process_name, std::uint32_t& out_process_id)
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
	std::vector<std::uint8_t> system_information_buffer;

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
				out_process_id = (std::uint64_t)(ULONG_PTR)current_info->UniqueProcessId;
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
			reinterpret_cast<std::uint8_t*>(current_info) + current_info->NextEntryOffset
			);
	}

	// In this case the process simply did not exist
	return true;
}

bool retrieveProcessIDByNameFallback(const std::string& process_name, std::vector<std::uint32_t>& process_ids)
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
	std::vector<std::uint8_t> system_information_buffer;

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
				process_ids.push_back((std::uint64_t)(ULONG_PTR)current_info->UniqueProcessId);
				//out_process_id = (std::uint64_t)(ULONG_PTR)current_info->UniqueProcessId;
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
			reinterpret_cast<std::uint8_t*>(current_info) + current_info->NextEntryOffset
			);
	}

	// In this case the process simply did not exist
	return true;
}

bool retrieveProcessIDByWindowName(const std::string& windowName, std::uint32_t& out_process_id)
{
	HWND hwnd = FindWindowA(NULL, windowName.c_str());

	if (hwnd == NULL) { return false; }

	DWORD processID = 0;
    GetWindowThreadProcessId(hwnd, &processID);

	if (processID == 0) { return false; }

	out_process_id = static_cast<std::uint32_t>(processID);
	return true;
}



}