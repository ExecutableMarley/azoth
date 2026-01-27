#include "../Process/CProcess.hpp"

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

    //Get Main ProcessImage
    auto mainImage = process.getProcessMainImage();
    std::cout << mainImage << "\n";

    // --- Memory Module ---
    // Provides read, write, allocate, protect, free and region enumeration
    auto& memory = process.getMemory();

    if (uint64_t someValue; memory.read(mainImage.baseAddress, &someValue))
    {
        std::cout << "Read value from main image: 0x" << someValue << "\n";
    }

    // --- Scanner Module ---
    // Pattern, signature and value scanning
    auto& scanner = process.getScanner();

    // Scan entire process memory
    Address globalMatch = scanner.findPatternEx(Pattern("FF FF FF FF ? ? ? ? FF FF"));
    if (globalMatch)
    {
        std::cout << "Pattern found globally at 0x" << globalMatch << "\n";
    }

    // Scan only the main module's memory region
    Address imageMatch  = scanner.findPatternEx(mainImage, Pattern("FF FF FF FF ? ? ? ? FF FF"));
    if (imageMatch)
    {
        std::cout << "Pattern found in main image at 0x" << imageMatch << "\n";
    }

    // --- Decoder Module
    auto& decoder = process.getDecoder();

    auto reRegions = memory.queryAllMemoryRegions(mainImage, ProtectionFilter::exact(EMemoryProtection::ReadExec));

    BYTE buffer[64];
    if (!reRegions.empty() && memory.read(reRegions[0].baseAddress, 64, buffer))
    {
        for (auto instr : decoder.range(buffer, sizeof(buffer), Address::fromPtr(&buffer)))
        {
            std::cout << instr.address << " " << decoder.wrap(instr) << std::endl;
        }
    }

    return 0;
}