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

    memory.write(remoteAddr, sourceValue);

    memory.read(remoteAddr, bufferValue);       //by reference
    bufferValue = memory.read<int>(remoteAddr); //as return

    std::cout << "Value read: " << bufferValue << "\n";

    // ----------------------------
    // Raw buffer read/write
    // ----------------------------

    memory.write(remoteAddr, sizeof(sourceValue), &sourceValue);
    memory.read(remoteAddr, sizeof(bufferValue), &bufferValue);

    // ----------------------------
    // String helpers
    // ----------------------------

    std::string sourceString = "1337";
    std::string bufferString;

    memory.write(remoteAddr, sourceString);
    memory.readString(remoteAddr, bufferString);

    std::cout << "String read: " << bufferString << "\n";

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