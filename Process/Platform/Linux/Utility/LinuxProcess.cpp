/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "LinuxProcess.hpp"
#include "../LinuxApiLink.hpp"

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

}