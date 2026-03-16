#include "../../third_party/doctest.h"

#include "Process/CProcess.hpp"

using namespace Azoth;

struct ScannerTestFixture
{
    CProcess process;
    CScannerModule* scanner;

    std::unique_ptr<BYTE[]> buffer;
    MemoryRange range;

    ScannerTestFixture()
        : process(Platform::createDefaultLayer())
    {
        REQUIRE(process.initialize());
        REQUIRE(process.attach(Platform::getPID()));

        scanner = &process.getScanner();

        buffer = std::make_unique<BYTE[]>(4096);
        std::memset(buffer.get(), 0xCC, 4096); // recognizable filler

        uint64_t base = reinterpret_cast<uint64_t>(buffer.get());
        range = MemoryRange(base, base + 4096);
    }
};

TEST_CASE_FIXTURE(ScannerTestFixture, "CScannerModule findPatternEx")
{
    SUBCASE("Finds first occurrence")
    {
        BYTE patternBytes[] = { 0xDE, 0xAD, 0xBE, 0xEF };
        std::memcpy(buffer.get() + 100, patternBytes, sizeof(patternBytes));

        Pattern pattern("DE AD BE EF");

        Address found = scanner->findPatternEx(range, pattern);

        CHECK(found == Address(reinterpret_cast<uint64_t>(buffer.get() + 100)));
    }

    SUBCASE("Returns 0 when not found")
    {
        Pattern pattern("AA BB CC DD");

        Address found = scanner->findPatternEx(range, pattern);

        CHECK(found == Address::null());
    }
}

TEST_CASE_FIXTURE(ScannerTestFixture, "CScannerModule signatureScanEx")
{
    //Todo:
}

TEST_CASE_FIXTURE(ScannerTestFixture, "CScannerModule findNextValue")
{
    SUBCASE("Raw value")
    {
        int value = 1337;

        std::memcpy(buffer.get() + 300, &value, sizeof(value));

        Address found = scanner->findNextValue(range, (BYTE*)&value, sizeof(value));

        CHECK(found == Address(reinterpret_cast<uint64_t>(buffer.get() + 300)));
    }

    SUBCASE("Typed value")
    {
        int value = 4242;
        std::memcpy(buffer.get() + 512, &value, sizeof(value));

        Address found = scanner->findNextValue(range, value);

        CHECK(found == Address(reinterpret_cast<uint64_t>(buffer.get() + 512)));
    }
}

TEST_CASE_FIXTURE(ScannerTestFixture, "CScannerModule findAllValues")
{
    int value = 7;

    std::memcpy(buffer.get() + 100, &value, sizeof(value));
    std::memcpy(buffer.get() + 200, &value, sizeof(value));
    std::memcpy(buffer.get() + 300, &value, sizeof(value));

    auto results = scanner->findAllValues(range, value);

    REQUIRE(results.size() == 3);

    bool allValid = std::all_of(
        results.begin(),
        results.end(),
        [&](Address a)
        {
            uint64_t offset = a.raw() - reinterpret_cast<uint64_t>(buffer.get());
            return offset == 100 || offset == 200 || offset == 300;
        }
    );

    CHECK(allValid);
}

TEST_CASE_FIXTURE(ScannerTestFixture, "CScannerModule scanForStrings")
{
    const char* s1 = "Hello";
    const char* s2 = "ScannerTest";

    std::memcpy(buffer.get() + 50,  s1, strlen(s1) + 1);
    std::memcpy(buffer.get() + 150, s2, strlen(s2) + 1);

    auto results = scanner->scanForStrings(range, 5);

    //Possible non-determinism. Could not reproduce failure consistently.
    REQUIRE(results.size() >= 2); 

    bool foundHello = false;
    bool foundScanner = false;

    for (const auto& [addr, str] : results)
    {
        if (str == "Hello")       foundHello = true;
        if (str == "ScannerTest") foundScanner = true;
    }

    CHECK(foundHello);
    CHECK(foundScanner);
}

TEST_CASE_FIXTURE(ScannerTestFixture, "CScannerModule scanForWideStrings")
{
    const char16_t* w1 = u"WideTest";

    std::memcpy(buffer.get() + 256, w1, (std::char_traits<char16_t>::length(w1) + 1) * sizeof(char16_t));

    auto results = scanner->scanForWideStrings(range, 4);

    REQUIRE(results.size() >= 1);

    bool found = false;

    for (const auto& [addr, str] : results)
    {
        if (str == u"WideTest")
            found = true;
    }

    CHECK(found);
}

TEST_CASE_FIXTURE(ScannerTestFixture, "CScannerModule scanForCodeCave")
{
    //Todo:
}