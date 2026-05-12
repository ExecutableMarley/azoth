/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "LinuxProcess.hpp"
#include "../LinuxApiLink.hpp"

#if __linux__


#include <elf.h>


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



class BinaryFile
{
public:
    BinaryFile(const std::string& path)
    {
        file.open(path, std::ios::binary);
    }

    bool valid() const { return file.is_open(); }

    template<typename T>
    bool read(uint64_t offset, T& out)
    {
        file.seekg(offset);
        return (bool)file.read(reinterpret_cast<char*>(&out), sizeof(T));
    }

    bool readBuffer(uint64_t offset, std::vector<char>& buffer, size_t size)
    {
        buffer.resize(size);
        file.seekg(offset);
        return (bool)file.read(buffer.data(), size);
    }

private:
    std::ifstream file;
};

// Todo: Template for 32/64 bit

struct ElfHeader
{
    uint8_t  ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct SectionHeader
{
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t addralign;
    uint64_t entsize;
};

struct Symbol
{
    uint32_t name;
    uint8_t  info;
    uint8_t  other;
    uint16_t shndx;
    uint64_t value;
    uint64_t size;
};

class ElfFile
{
public:
    explicit ElfFile(const std::string& path)
        : file(path)
    {}

    bool parse() {
        if (!file.valid()) return false;

        if (!file.read(0, header)) return false;

        // Basic validation
        if (header.ident[0] != 0x7F || header.ident[1] != 'E' ||
            header.ident[2] != 'L'  || header.ident[3] != 'F')
            return false;

        return loadSectionHeaders();
    }

    struct ParsedSymbol {
        std::string name;
        uint64_t value;
        uint8_t info;
        uint16_t shndx;
    };

    std::vector<ParsedSymbol> getSymbols(bool includeSymtab = false) {
        std::vector<ParsedSymbol> result;

        extractSymbols(".dynsym", result);

        if (includeSymtab)
            extractSymbols(".symtab", result);

        return result;
    }

private:
    BinaryFile file;
    ElfHeader header{};
    std::vector<SectionHeader> sections;
    std::vector<char> shstrtab;

    bool loadSectionHeaders()
    {
        sections.resize(header.shnum);

        for (uint16_t i = 0; i < header.shnum; ++i)
        {
            file.read(header.shoff + i * sizeof(SectionHeader), sections[i]);
        }

        // Load section name table
        const auto& shstr = sections[header.shstrndx];
        return file.readBuffer(shstr.offset, shstrtab, shstr.size);
    }

    const SectionHeader* findSection(const std::string& name) const
    {
        for (const auto& sec : sections)
        {
            const char* secName = &shstrtab[sec.name];
            if (name == secName)
                return &sec;
        }
        return nullptr;
    }

    void extractSymbols(const std::string& symName,
                        std::vector<ParsedSymbol>& out)
    {
        const SectionHeader* symSec = findSection(symName);
        if (!symSec) return;

        const SectionHeader& strSec = sections[symSec->link];

        std::vector<char> strtab;
        file.readBuffer(strSec.offset, strtab, strSec.size);

        size_t count = symSec->size / symSec->entsize;

        for (size_t i = 0; i < count; ++i)
        {
            Symbol sym{};
            file.read(symSec->offset + i * sizeof(Symbol), sym);

            if (sym.name == 0 || sym.shndx == 0)
                continue;

            if (sym.name >= strtab.size())
                continue;

            const char* name = &strtab[sym.name];

            if (!name || !*name)
                continue;

            out.push_back({
                std::string(name),
                sym.value,
                sym.info,
                sym.shndx
            });
        }
    }
};

PlatformErrorState retrieveSymbols(const ProcessImage& image, std::vector<ImageSymbol>& symbols)
{
    ElfFile elf(image.path);

    if (!elf.parse())
        return { EPlatformError::MalformedData, 0 };

    auto parsed = elf.getSymbols(false); // dynsym only

    int count = 0;
    for (const auto& sym : parsed)
    {
        uint8_t bind = sym.info >> 4;

        bool isImport = (sym.shndx == SHN_UNDEF);
        bool isExport = !isImport && (bind == STB_GLOBAL || bind == STB_WEAK);

        if (!isImport && !isExport)
            continue;

        ImageSymbol out{};
        out.modName = image.name;
        out.name = sym.name;
        out.address = image.baseAddress + sym.value; // Todo: Import symbols resolve
        out.source = isImport ? SymbolSource::Import : SymbolSource::Export;
        out.index = count++;
        out.forwarder = ""; // leave empty
        symbols.push_back(std::move(out));
    }

    return { EPlatformError::Success, 0 };
}



} // namespace Azoth

#endif