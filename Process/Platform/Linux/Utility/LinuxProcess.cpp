/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "LinuxProcess.hpp"
#include "../LinuxApiLink.hpp"

#if __linux__

namespace Azoth
{


std::unique_ptr<IPlatformLink> Platform::createDefaultLayer()
{
    return std::make_unique<LinuxLink>();
}

uint32_t Platform::getPID()
{
    return getpid();
}

PlatformErrorState sendSignal(pid_t pid, int signal)
{
    if (kill(pid, signal) == 0)
    {
        return PlatformErrorState{ EPlatformError::Success, 0 };
    }

    if (errno == ESRCH) return PlatformErrorState{ EPlatformError::ResourceNotFound, 0 };
    if (errno == EPERM) return PlatformErrorState{ EPlatformError::AccessDenied, 0 };
    return PlatformErrorState{ EPlatformError::InternalError, (uint64_t)errno };
}



EPlatformError getProcessMappedBinaries(uint32_t pid, std::unordered_map<std::string, ProcessImage>& outBinaries)
{
    std::ifstream mapsFile("/proc/" + std::to_string(pid) + "/maps");
    if (!mapsFile.is_open())
        return EPlatformError::ResourceNotFound;

    std::string line;
    while (std::getline(mapsFile, line))
    {
        std::istringstream iss(line);
        std::string addr, perms, offset, dev, inode, path;
        if (!(iss >> addr >> perms >> offset >> dev >> inode))
            continue;
        std::getline(iss, path);
        if (!path.empty() && path[0] == ' ')
            path = path.substr(1);

        // Only files with a path
        if (path.empty())
            continue;

        auto dash = addr.find('-');
        if (dash == std::string::npos)
            continue;

        uint64_t start = std::stoull(addr.substr(0, dash) , nullptr, 16);
        uint64_t end   = std::stoull(addr.substr(dash + 1), nullptr, 16);
        size_t size = end - start;

        // filename as key
        std::string name = std::filesystem::path(path).filename().string();

        // Check if we already found a mapping
        auto it = outBinaries.find(name);
        if (it == outBinaries.end())
        {
            outBinaries[name] = ProcessImage(start, size, name, path);
        }
        else
        {
            uint64_t currentStart = it->second.baseAddress;
            uint64_t currentEnd   = it->second.baseAddress + it->second.size;

            uint64_t newStart = std::min(currentStart, start);
            uint64_t newEnd   = std::max(currentEnd, end);

            it->second.baseAddress = newStart;
            it->second.size        = newEnd - newStart;
        }
    }
    return EPlatformError::Success;
}


EPlatformError parseProcessMaps(uint32_t pid, std::vector<MemoryRegion>& outRegions, std::unordered_map<std::string, ProcessImage>& outBinaries)
{
    outRegions.clear();
    outBinaries.clear();

    std::ifstream mapsFile("/proc/" + std::to_string(pid) + "/maps");
    if (!mapsFile.is_open())
        return EPlatformError::ResourceNotFound;

    std::string line;
    while (std::getline(mapsFile, line))
    {
        std::istringstream iss(line);
        std::string addr, perms, offset, dev, inode, path;
        if (!(iss >> addr >> perms >> offset >> dev >> inode))
            continue;
        std::getline(iss, path);
        //if (!path.empty() && path[0] == ' ')
        //    path = path.substr(1);
        if (!path.empty())
        {
            const auto first = path.find_first_not_of(' ');
            if (first != std::string::npos)
                path.erase(0, first);
            else
                path.clear();
        }

        auto dash = addr.find('-');
        if (dash == std::string::npos)
            continue;

        uint64_t start = std::stoull(addr.substr(0, dash) , nullptr, 16);
        uint64_t end   = std::stoull(addr.substr(dash + 1), nullptr, 16);
        uint64_t size = (end > start) ? static_cast<size_t>(end - start) : 0;

        if (size == 0) // skip zero-sized regions
            continue;

        EMemoryProtection prot = EMemoryProtection::None;
        if (perms.size() >= 1 && perms[0] == 'r') prot |= EMemoryProtection::Read;
        if (perms.size() >= 2 && perms[1] == 'w') prot |= EMemoryProtection::Write;
        if (perms.size() >= 3 && perms[2] == 'x') prot |= EMemoryProtection::Execute;

        EMemoryType type = EMemoryType::Unknown;
        if (!path.empty() && path[0] != '[')
        {
            if ((prot & EMemoryProtection::Execute) != EMemoryProtection::None)
                type = EMemoryType::Image;
            else
                type = EMemoryType::Mapped;
        }
        else
        {
            if (perms.size() >= 4 && perms[3] == 'p')
                type = EMemoryType::Private;
            else
                type = EMemoryType::Unknown;
        }

        MemoryRegion region;
        region.baseAddress = start;
        region.regionSize  = size;
        region.protection  = prot;
        region.state       = EMemoryState::Committed;
        region.type        = type;

        outRegions.push_back(region);

        // If path exists, add/merge into outBinaries
        if (!path.empty() && path[0] == '/')
        {
            std::string name = std::filesystem::path(path).filename().string();
            auto it = outBinaries.find(name);
            if (it == outBinaries.end())
            {
                outBinaries[name] = ProcessImage(start, size, name, path);
            }
            else
            {
                uint64_t currentStart = it->second.baseAddress;
                uint64_t currentEnd   = it->second.baseAddress + it->second.size;

                uint64_t newStart = std::min(currentStart, start);
                uint64_t newEnd   = std::max(currentEnd, end);

                it->second.baseAddress = newStart;
                it->second.size        = newEnd - newStart;
            }
        }
    }

    // sort regions by base
    std::sort(outRegions.begin(), outRegions.end(), [](const MemoryRegion& a, const MemoryRegion& b){ return a.baseAddress < b.baseAddress; });

    return EPlatformError::Success;
}


bool getMapsWriteTime(uint32_t pid, std::filesystem::file_time_type& outTime)
{
    try
    {
        //Consider using stat directly to avoid filesystem overhead?
        std::filesystem::path p = std::filesystem::path("/proc") / std::to_string(pid) / "maps";
        outTime = std::filesystem::last_write_time(p);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool readStartTime(pid_t pid, uint64_t& startTime)
{
    std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
    if (!file)
        return false;

    std::string line;
    std::getline(file, line);

    auto rparen = line.rfind(')');
    if (rparen == std::string::npos)
        return false;

    std::istringstream iss(line.substr(rparen + 2));

    std::string dummy;

    // skip 19 fields? (Verify this)
    for (int i = 0; i < 19; ++i)
        iss >> std::ws >> dummy;

    iss >> startTime;
    return !iss.fail();
}


}


#endif