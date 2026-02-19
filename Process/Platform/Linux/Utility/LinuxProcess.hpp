/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <string>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>

#include "../../EPlatformError.hpp"
#include "../../../Types/EProcessArchitecture.hpp"
#include "../../../Types/EProcessPrivilegeLevel.hpp"

#include "../../../Core/ProcessImage.hpp"
#include "../../../Core/MemoryRegion.hpp"


namespace Azoth
{


PlatformErrorState sendSignal(pid_t pid, int signal);


EPlatformError getProcessMappedBinaries(uint32_t pid, std::unordered_map<std::string, ProcessImage>& outBinaries);

// Parse /proc/<pid>/maps and produce both memory region list and mapped binaries.
EPlatformError parseProcessMaps(uint32_t pid, std::vector<MemoryRegion>& outRegions, std::unordered_map<std::string, ProcessImage>& outBinaries);

// Get last write time of /proc/<pid>/maps. Returns true on success and fills outTime.
bool getMapsWriteTime(uint32_t pid, std::filesystem::file_time_type& outTime);


}