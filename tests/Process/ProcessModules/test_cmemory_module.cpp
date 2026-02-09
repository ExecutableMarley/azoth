#include "../../third_party/doctest.h"

#include "Process/CProcess.hpp"

using namespace Azoth;

struct MemoryTestFixture
{
    CProcess process;
    CMemoryModule* memory;

    std::unique_ptr<BYTE[]> buffer;
    uint64_t remoteAddr;

    MemoryTestFixture() : process(Platform::createDefaultLayer())
    {
        REQUIRE(process.initialize());
        REQUIRE(process.attach(Platform::getPID()));

        memory = &process.getMemory();

        buffer = std::make_unique<BYTE[]>(4096);
        remoteAddr = reinterpret_cast<uint64_t>(buffer.get());

        std::memset(buffer.get(), 0, 4096);
    }
};

TEST_CASE_FIXTURE(MemoryTestFixture, "CMemoryModule Basic Read/Write Operations")
{
    SUBCASE("Typed Read/Write")
    {
        int expected = 4242;
        int outputByRef = 0;

        // Test Write
        REQUIRE(memory->write(remoteAddr, expected));

        // Test Read by Reference
        REQUIRE(memory->read(remoteAddr, outputByRef));
        CHECK(outputByRef == expected);

        // Test Read by Return
        int outputByReturn = memory->read<int>(remoteAddr);
        CHECK(outputByReturn == expected);
    }

    SUBCASE("Raw Buffer Read/Write")
    {
        struct Data { int a; float b; char c[4]; };
        
        Data send = { 10, 20.5f, "abc" };
        Data receive = { 0 };

        REQUIRE(memory->write(remoteAddr, sizeof(Data), &send));
        REQUIRE(memory->read(remoteAddr, sizeof(Data), &receive));

        CHECK(receive.a == send.a);
        CHECK(receive.b == send.b);
        CHECK(std::string(receive.c) == "abc");
    }
}

TEST_CASE_FIXTURE(MemoryTestFixture, "CMemoryModule String Operations")
{
    SUBCASE("Reading std::string")
    {
        char rawString[]     = "Hello Doctest";
        uint64_t remoteStringAddr = reinterpret_cast<uint64_t>(rawString);

        std::string buf;
        // Test readString into reference
        REQUIRE(memory->readString(remoteStringAddr, buf));
        CHECK(buf == "Hello Doctest");

        // Test readString by return
        std::string buf2 = memory->readString(remoteStringAddr);
        CHECK(buf2 == "Hello Doctest");
    }

    //Todo: There is currently no readWideString method
    //For now we do not check std::wstring
    //wchar_t rawWString[] = L"Hello Doctest";
    //uint64_t remoteWStringAddr = reinterpret_cast<uint64_t>(rawWString);

    SUBCASE("Writing std::string")
    {
        std::string src = "Hello Memory!";
        std::string dst;

        REQUIRE(memory->write(remoteAddr, src));
        REQUIRE(memory->readString(remoteAddr, dst));

        CHECK(dst == src);
    }

    SUBCASE("String max size constraints")
    {
        char rawString[]          = "Hello Doctest";
        uint64_t remoteStringAddr = reinterpret_cast<uint64_t>(rawString);

        std::string buf;
        REQUIRE(memory->readString(remoteStringAddr, buf, 6)); 
        CHECK(buf.length() <= 6); 
    }
}

TEST_CASE_FIXTURE(MemoryTestFixture, "CMemoryModule Query Operations")
{
    SUBCASE("Memory Querying")
    {
        MemoryRegion region;

        REQUIRE(memory->queryMemory(remoteAddr, region));

        CHECK(region.baseAddress != 0);
        CHECK(region.regionSize > 0);

        CHECK(remoteAddr >= region.baseAddress);
        CHECK(remoteAddr  < region.baseAddress + region.regionSize);
    }

    SUBCASE("Query All Regions")
    {
        auto regions = memory->queryAllMemoryRegions(MemoryRange::max_range_64bit());

        // Very basic checks
        REQUIRE(regions.size() > 0);
        
        bool allValid = true;
        for (const auto& region : regions)
        {
            if (region.regionSize == 0 || region.baseAddress == 0)
            {
                allValid = false;
                break;
            }
        }
        CHECK(allValid);
    }
}