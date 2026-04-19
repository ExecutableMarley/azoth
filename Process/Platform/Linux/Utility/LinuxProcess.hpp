/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#if __linux__

#include "../../EPlatformError.hpp"
#include "../../../Types/EProcessArchitecture.hpp"
#include "../../../Types/EProcessPrivilegeLevel.hpp"
#include "../../../Core/ProcessImage.hpp"
#include "../../../Core/MemoryRegion.hpp"

#include <string>
#include <sstream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <sys/mman.h>


namespace Azoth
{


PlatformErrorState sendSignal(pid_t pid, int signal);


EPlatformError getProcessMappedBinaries(std::uint32_t pid, std::unordered_map<std::string, ProcessImage>& outBinaries);

// Parse /proc/<pid>/maps and produce both memory region list and mapped binaries.
EPlatformError parseProcessMaps(std::uint32_t pid, std::vector<MemoryRegion>& outRegions, std::unordered_map<std::string, ProcessImage>& outBinaries);

// Get last write time of /proc/<pid>/maps. Returns true on success and fills outTime.
bool getMapsWriteTime(std::uint32_t pid, std::filesystem::file_time_type& outTime);

inline int toProt(EMemoryProtection p)
{
    int prot = 0;
    if (hasFlag(p, EMemoryProtection::Read))    prot |= PROT_READ;
    if (hasFlag(p, EMemoryProtection::Write))   prot |= PROT_WRITE;
    if (hasFlag(p, EMemoryProtection::Execute)) prot |= PROT_EXEC;
    return prot;
}

PlatformErrorState retrieveSymbols(const ProcessImage& image, std::vector<ImageSymbol>& symbols);


}


#endif