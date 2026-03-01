#include "Process/CProcess.hpp"

#include <regex>

using namespace Azoth;

// ----------------------------
// Prepare test data in local process memory
// ----------------------------

const char password[] = "S3cr3t!Passw0rd";
const char someString[] = "Hello World!";

int testValue = 1337;

BYTE testBuffer[16] = {
    0xAA, 0xBB, 0xCC, 0xDD,
    0x11, 0x22, 0x33, 0x44,
    0x55, 0x66, 0x77, 0x88,
    0x99, 0x00, 0xAB, 0xCD
};

template<typename T>
inline void anchor(T const& value)
{
    asm volatile("" : : "g"(&value) : "memory");
}

int main()
{
    // Pretend fake use. 'Should' stop the compiler from optimizing these away
    anchor(password); anchor(someString); anchor(testValue); anchor(testBuffer);


    CProcess process(Platform::createDefaultLayer());

    // Initialize internal state
    if (!process.initialize())
        return -1;

    // Attach to the process
    if (!process.attach(Platform::getPID()))
        return -1;

    std::cout << "Attached to process\n";

    ProcessImage mainImage = process.getProcessMainImage();

    auto& memory  = process.getMemory();
    auto& scanner = process.getScanner();


    // ----------------------------
    // Pattern scanning
    // ----------------------------

    //Performs a signature-based search (AOB) to locate specific byte sequences

    Pattern pattern("AA BB CC DD ? ? 33 44");

    Address patternMatch = scanner.findPatternEx(mainImage, pattern, ProtectionFilter::exact(EMemoryProtection::ReadWrite));

    if (patternMatch)
    {
        std::cout << "Pattern found at: " << patternMatch << "\n";
    }

    // Find all pattern matches

    auto patternMatches = scanner.findAllPatternEx(mainImage, pattern);

    std::cout << "Total pattern matches: " 
              << patternMatches.size() << "\n";

    for (auto addr : patternMatches)
    {
        std::cout << "  -> " << addr << "\n";
    }

    // ----------------------------
    // Value scanning (typed)
    // ----------------------------

    // Executes a type-safe search for a specific trivially copyable value (e.g., int)

    Address valueMatch = scanner.findNextValue<int>(mainImage, testValue);

    if (valueMatch)
    {
        std::cout << "Value " << testValue << " found at: " << valueMatch << "\n";
    }

    // ----------------------------
    // Raw data scanning
    // ----------------------------

    // Scans for an exact byte-for-byte replica of a local buffer, useful for 
    // locating structures, headers, or serialized data objects in memory.

    auto rawMatches = scanner.findAllValues(mainImage, (BYTE*)testBuffer, sizeof(testBuffer));

    std::cout << "Raw buffer matches: " << rawMatches.size() << "\n";

    for (auto addr : rawMatches)
    {
        std::cout << "  -> " << addr << "\n";
    }


    // ----------------------------
    // Scan for a code cave
    // ----------------------------

    // Searches for a 64-byte contiguous block of unused memory (e.g., 0x00 or padding) 
    // within the process image, additionally enforcing a 16-byte alignment.

    Address codeCave = scanner.scanForCodeCave(mainImage, 64, 16);
    
    if (codeCave)
    {
        std::cout << "Found code cave (64 bytes) at: " 
                  << codeCave << "\n";
    }
    else
    {
        std::cout << "No suitable code cave found\n";
    }

    // ----------------------------
    // Scan for strings in module memory
    // ----------------------------

    // Identifies candidate passwords by scanning Read memory for strings 
    // (min length 8) and filtering results against a standard password regex.

    auto stringResults = scanner.scanForStrings(mainImage, 8, ProtectionFilter::require(EMemoryProtection::Read));
    
    std::cout << "Found " << stringResults.size() << " strings in main image\n";

    // Min 8 chars, at least one uppercase, one lowercase, one number, and one special character (@$!%*?&).
    std::regex passwordRegex(R"(^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]{8,}$)");

    std::vector<std::pair<Address, std::string>> possiblePasswords;
    std::copy_if(stringResults.begin(), stringResults.end(), std::back_inserter(possiblePasswords), 
        [&passwordRegex](std::pair<Address, std::string> pair) { return std::regex_match(pair.second, passwordRegex); });

    if (possiblePasswords.empty())
    {
        std::cout << "No password-like strings found\n";
    }
    else
    {
        std::cout << "Possible passwords found:\n";

        for (const auto& result : possiblePasswords)
        {
            std::cout << "  [" << result.first << "] "
                      << result.second << "\n";
        }
    }

    return 0;
}