#pragma once

#include <string>
#include <stdint.h>
#include <vector>

#include "../../EPlatformError.hpp"
#include "../../../Types/EProcessArchitecture.hpp"
#include "../../../Types/EProcessPrivilegeLevel.hpp"

#undef UNICODE
#undef _UNICODE
#include <windows.h>

namespace Azoth
{

class ProcessImage;

PlatformErrorState retrieveProcessIDByName(const std::string& procName, std::uint32_t& out_process_id);

PlatformErrorState retrieveProcessIDByName(const std::string& procName, std::vector<std::uint32_t>& out_process_ids);

bool retrieveProcessIDByNameFallback(const std::string& process_name, std::uint32_t& out_process_id);

bool retrieveProcessIDByNameFallback(const std::string& process_name, std::vector<std::uint32_t>& out_process_ids);

PlatformErrorState retrieveProcessIDByWindowName(const std::string& windowName, std::uint32_t& out_process_id);

//

PlatformErrorState retrieveProcessMainImage(uint32_t pid, ProcessImage& procImage);

PlatformErrorState retrieveProcessImage(uint32_t pid, const std::string& imageName, ProcessImage& procImage);

PlatformErrorState retrieveAllProcessImages(uint32_t pid, std::vector<ProcessImage>& procImages);

//

PlatformErrorState retrieveProcessPrivilegeLevel(HANDLE processHandle, EProcessPrivilegeLevel& outLevel);

PlatformErrorState retrieveProcessArchitecture(HANDLE processHandle, EProcessArchitecture& outArch);

PlatformErrorState retrieveProcessName(HANDLE processHandle, std::string& outProcessName);

PlatformErrorState retrieveProcessPath(HANDLE processHandle, std::string& outProcessPath);

}