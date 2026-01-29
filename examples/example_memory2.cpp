#include "Process/CProcess.hpp"

using namespace Azoth;

int main()
{
    CProcess process(Platform::createDefaultLayer());

    // Initialize internal state
    if (!process.initialize())
        return -1;

    // Attach to the process
    if (!process.attach(Platform::getPID()))
        return -1;

    std::cout << "Attached to process\n";

    auto& memory = process.getMemory();


    // ----------------------------
    // Virtual memory allocation
    // ----------------------------

    Address remoteAddress = memory.virtualAllocate(4096, EMemoryProtection::RWX);

    if (!remoteAddress)
    {
        //std::cerr << "Virtual allocation failed: " << memory.getLastError() << "\n";
        std::cerr << "Virtual allocation failed: " << Error::getLast() << "\n";
    }
    else
    {
        std::cout << "Allocated remote memory at: " << remoteAddress << "\n";
    }
    

    // ----------------------------
    // Change memory protection
    // ----------------------------

    EMemoryProtection oldProtect;

    //Old protection ptr is optional
    if (!memory.virtualProtect(remoteAddress, 4096, EMemoryProtection::ReadWrite, &oldProtect))
    {
        std::cerr << "Virtual Protect failed: " << Error::getLast() << "\n";
    }
    else
    {
        std::cout << "Protection changed (old = " << oldProtect << ")\n";
    }
    

    // Restore Original protection
    memory.restoreProtection(remoteAddress);
    std::cout << "Protection restored\n";

    // ----------------------------
    // Patch read-only memory safely
    // ----------------------------

    uint64_t newValue = 0xDEADBEEFCAFEBABE;

    // Automatically makes region writable and restores protection after
    if (!memory.patchReadOnlyMemory(remoteAddress, sizeof(newValue), (BYTE*)&newValue))
    {
        std::cerr << "patch read only memory failed: " << Error::getLast() << "\n";
    }
    else
    {
        std::cout << "Patched memory with value: 0x" << std::hex << newValue << std::dec << "\n";
    }

    // Restores original memory
    memory.restoreReadOnlyMemory(remoteAddress);
    std::cout << "Original memory restored\n";

    // ----------------------------
    // Free allocated memory
    // ----------------------------

    // equivalent to memory.virtualFree
    memory.restoreAllocation(remoteAddress);
    std::cout << "Remote memory freed\n";

    return 0;
}