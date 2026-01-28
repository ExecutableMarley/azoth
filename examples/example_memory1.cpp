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

    // Setup a buffer and treat it like remote memory
    std::unique_ptr someBuffer = std::make_unique<BYTE[]>(4096);
    Address remoteAddr = Address::fromPtr(someBuffer.get());

    // ----------------------------
    // Typed read/write
    // ----------------------------

    int sourceValue = 1337;
    int bufferValue = 0;

   if (memory.write(remoteAddr, sourceValue))
   {
        std::cout << "Value written to remote Address: " << sourceValue << "\n";
   }

    if (memory.read(remoteAddr, bufferValue))       //by reference
    {
        std::cout << "Value read by reference: " << bufferValue << "\n";
    }

    bufferValue = memory.read<int>(remoteAddr); //as return

    std::cout << "Value read by return: " << bufferValue << "\n";

    // ----------------------------
    // Raw buffer read/write
    // ----------------------------

    if (memory.write(remoteAddr, sizeof(sourceValue), &sourceValue))
    {
        std::cout << "Value written by buffer: " << sourceValue << "\n";
    }

    if (memory.read(remoteAddr, sizeof(bufferValue), &bufferValue))
    {
        std::cout << "Value read by buffer: " << bufferValue << "\n";   
    }

    // ----------------------------
    // String helpers
    // ----------------------------

    std::string sourceString = "1337";
    std::string bufferString;

    if (memory.write(remoteAddr, sourceString))
    {
        std::cout << "String written to remote address " << sourceString << "\n";
    }

    if (memory.readString(remoteAddr, bufferString))
    {
        std::cout << "String read from remote address: " << bufferString << "\n";   
    }

    // ----------------------------
    // Query memory region info
    // ----------------------------

    MemoryRegion region;
    if (memory.queryMemory(remoteAddr, region))
    {
        std::cout << region << "\n";

        // Example members:
        // region.baseAddress
        // region.regionSize
        // region.protection
        // region.state
        // region.type
    }

    // ----------------------------
    // Enumerate all memory regions in range
    // ----------------------------

    std::vector<MemoryRegion> regions = memory.queryAllMemoryRegions(
        MemoryRange::max_range_64bit()
    );

    std::cout << "Total regions: " << regions.size() << "\n";
    

    return 0;
}