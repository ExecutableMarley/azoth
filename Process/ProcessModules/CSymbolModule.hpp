/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../Types/Address.hpp"
#include "../Types/MemoryRange.hpp"
#include "../Core/ProcessImage.hpp"
#include "../Platform/IPlatformLink.hpp"

#include <vector>
#include <string>

namespace Azoth
{


class CProcess;


/**
 * @brief Lightweight handle referencing a module in the process image cache.
 * 
 * Contains the index into the cache and the base address for quick validation.
 */
struct ModuleEntryHandle
{
    static constexpr size_t npos = static_cast<size_t>(-1);

    size_t index = npos;
    Address base = 0;

    ModuleEntryHandle() = default;
    ModuleEntryHandle(size_t i, Address b) : index(i), base(b) {}

    bool valid() const { return index != npos; }
};

/**
 * @brief Provides module and symbol resolution for a target process.
 *
 * Maintains a cache of loaded process modules and enables lookup by
 * address or name. Supports resolving symbols within modules, including
 * name-based and address-based queries.
 */
class CSymbolModule
{
public:
    CSymbolModule(CProcess* process, IPlatformLink* platformLink)
        : _backPtr(process), _platformLink(platformLink) {}

public:
    CSymbolModule(const CSymbolModule&) = delete;
    CSymbolModule& operator=(const CSymbolModule&) = delete;

public:
    
    // Module Cache

    bool refreshModuleCache()
    {
        std::vector<ProcessImage> images;
        if (!_platformLink->getAllProcessImages(images)) { return false; }

        std::sort(images.begin(), images.end(),
            [](const ProcessImage& a, const ProcessImage& b) { return a.baseAddress < b.baseAddress; });

        size_t i = 0; // new images
        size_t j = 0; // cache

        while (i < images.size() && j < _processImageCache.size())
        {
            const auto& newImg = images[i];
            const auto& cachedImg = _processImageCache[j].image;

            if (newImg == cachedImg)
            {
                // Same module
                ++i;
                ++j;
            }
            else if (newImg.baseAddress < cachedImg.baseAddress)
            {
                // New module appeared
                _processImageCache.insert(_processImageCache.begin() + j,
                    ModuleSymbolEntry{ newImg, {}, false });

                ++i;
                ++j;
            }
            else
            {
                // Cached module disappeared
                _processImageCache.erase(_processImageCache.begin() + j);
            }
        }

        if (i < images.size())
        {
            while (i < images.size())
            {
                _processImageCache.push_back(ModuleSymbolEntry{ images[i], {}, false });
                ++i;
            }
        }
        else
        {
            while (j < _processImageCache.size())
            {
                _processImageCache.erase(_processImageCache.begin() + j);
            }
        }
        return true;
    }

    size_t getModuleCount() const
    {
        return _processImageCache.size();
    }

    // Module Lookup

    bool getModule(size_t index, ProcessImage& outImage)
    {
        if (index < _processImageCache.size())
        {
            outImage = _processImageCache[index].image;
            return true;
        }
        return false;
    }

    bool findModuleByAddress(Address runtimeAddress, ProcessImage& outImage)
    {
        //Binary search for the module containing runtimeAddress
        auto it = std::upper_bound(_processImageCache.begin(), _processImageCache.end(), runtimeAddress, [](const Address& addr, const ModuleSymbolEntry& entry) {
            return addr < entry.image.baseAddress;
        });

        if (it == _processImageCache.begin())
            return false;

        --it; // Step back to the module that is <= runtimeAddress

        if (runtimeAddress >= it->image.baseAddress && runtimeAddress < it->image.baseAddress + it->image.size)
        {
            outImage = it->image;
            return true;
        }

        return false;
    }

    bool findModuleByName(const std::string_view moduleName, ProcessImage& outImage)
    {
        auto it = std::find_if(_processImageCache.begin(), _processImageCache.end(), [&moduleName](const ModuleSymbolEntry& mod) {
            return mod.image.name == moduleName;
        });

        if (it == _processImageCache.end()) return false;

        outImage = it->image;
        return true;
    }

    // Handles

    // Return a lightweight handle for the module that would contain runtimeAddress.
    // The handle can be passed between interfaces to avoid repeating the binary search.
    ModuleEntryHandle getModuleHandle(Address runtimeAddress)
    {
        auto it = std::upper_bound(_processImageCache.begin(), _processImageCache.end(), runtimeAddress, [](const Address& addr, const ModuleSymbolEntry& entry) {
            return addr < entry.image.baseAddress;
        });

        if (it == _processImageCache.begin())
            return ModuleEntryHandle();

        --it; // Step back to the module that is <= runtimeAddress

        if (runtimeAddress >= it->image.baseAddress && runtimeAddress < it->image.baseAddress + it->image.size)
        {
            size_t idx = std::distance(_processImageCache.begin(), it);
            return ModuleEntryHandle(idx, it->image.baseAddress);
        }
        return ModuleEntryHandle();
    }

    ModuleEntryHandle getModuleHandle(const std::string_view moduleName)
    {
        auto it = std::find_if(_processImageCache.begin(), _processImageCache.end(), [&moduleName](const ModuleSymbolEntry& mod) {
            return mod.image.name == moduleName;
        });

        if (it == _processImageCache.end()) return ModuleEntryHandle();

        size_t idx = std::distance(_processImageCache.begin(), it);
        return ModuleEntryHandle(idx, it->image.baseAddress);
    }
    
private:

    class ModuleSymbolEntry
    {
    public:
        ProcessImage image;
        std::vector<ImageSymbol> symbols;
        
        bool isParsed = false;
        
        bool getSymbolByAddress(Address runtimeAddress, bool exactMatch, ImageSymbol& outSymbol)
        {
            if (!isParsed) return false;
        
            auto it = std::upper_bound(symbols.begin(), symbols.end(), runtimeAddress, [](const Address& addr, const ImageSymbol& sym) {
                return addr < sym.address;
            });
        
            if (it == symbols.begin()) return false;
        
            --it; // Step back to the symbol that is <= runtimeAddress
        
            size_t symbolSize = std::next(it) != symbols.end() ? std::next(it)->address - it->address : 1; // Assume size 1 for the last symbol
        
            if (exactMatch)
            {
                if (it->address == runtimeAddress)
                {
                    outSymbol = *it;
                    return true;
                }
                return false;
            }
            else if (runtimeAddress >= it->address && runtimeAddress < it->address + symbolSize)
            {
                outSymbol = *it;
                return true;
            }
        
            return false;
        }
    
        bool getSymbolByName(const std::string_view name, ImageSymbol& outSymbol)
        {
            //Todo: O(n)
            if (!isParsed) return false;
        
            auto it = std::find_if(symbols.begin(), symbols.end(), [&name](const ImageSymbol& sym) {
                return sym.name == name;
            });
        
            if (it == symbols.end()) return false;
        
            outSymbol = *it;
            return true;
        }
    };

    // Validate a handle and return a pointer to the cached ModuleSymbolEntry, or nullptr if invalid.
    ModuleSymbolEntry* getModuleFromHandle(const ModuleEntryHandle& handle)
    {
        if (!handle.valid())
            return nullptr;

        if (handle.index < _processImageCache.size() && _processImageCache[handle.index].image.baseAddress == handle.base)
            return &_processImageCache[handle.index];

        auto it = std::find_if(_processImageCache.begin(), _processImageCache.end(), [&handle](const ModuleSymbolEntry& mod) {
            return mod.image.baseAddress == handle.base;
        });

        if (it == _processImageCache.end()) return nullptr;

        return &(*it);
    }

public:

    // Symbol lookup

    bool findSymbolByAddress(Address runtimeAddress, bool exactMatch, ImageSymbol& outSymbol)
    {
        ModuleEntryHandle handle = getModuleHandle(runtimeAddress);
        if (!handle.valid())
            return false;

        return findSymbolByAddress(handle, runtimeAddress, exactMatch, outSymbol);
    }

    bool findSymbolByAddress(const ModuleEntryHandle& handle, Address runtimeAddress, bool exactMatch, ImageSymbol& outSymbol)
    {
        ModuleSymbolEntry* entry = getModuleFromHandle(handle);
        if (!entry)
            return false;

        if (entry->isParsed)
        {
            return entry->getSymbolByAddress(runtimeAddress, exactMatch, outSymbol);
        }
        else
        {
            //Parse module
            _platformLink->getSymbols(entry->image, entry->symbols);
            entry->isParsed = true;

            std::sort(entry->symbols.begin(), entry->symbols.end(), [](const ImageSymbol& a, const ImageSymbol& b) {
                return a.address < b.address;
            });

            return entry->getSymbolByAddress(runtimeAddress, exactMatch, outSymbol);
        }
    }

    bool findSymbolByName(const ModuleEntryHandle& handle, const std::string_view moduleName, ImageSymbol& outSymbol)
    {
        ModuleSymbolEntry* entry = getModuleFromHandle(handle);
        if (!entry)
            return false;

        if (entry->isParsed)
        {
            return entry->getSymbolByName(moduleName, outSymbol);
        }
        else
        {
            //Parse module
            _platformLink->getSymbols(entry->image, entry->symbols);
            entry->isParsed = true;

            std::sort(entry->symbols.begin(), entry->symbols.end(), [](const ImageSymbol& a, const ImageSymbol& b) {
                return a.address < b.address;
            });

            return entry->getSymbolByName(moduleName, outSymbol);
        }
    }

    bool findSymbolByName(const std::string_view moduleName, const std::string_view symbolName, ImageSymbol& outSymbol)
    {
        ModuleEntryHandle handle = getModuleHandle(moduleName);

        return findSymbolByName(handle, symbolName, outSymbol);
    }

    bool findSymbol(const std::string_view combinedName, ImageSymbol& outSymbol)
    {
        size_t pos = combinedName.find('!');

        if (pos == std::string_view::npos) return false;

        std::string_view mod = combinedName.substr(0, pos);
        std::string_view sym = combinedName.substr(pos + 1);

        return findSymbolByName(mod, sym, outSymbol);
    }

private:
    CProcess* _backPtr;
    IPlatformLink* _platformLink;

    //Keep those sorted
    std::vector<ModuleSymbolEntry> _processImageCache;
};


}