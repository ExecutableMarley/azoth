#include "../../third_party/doctest.h"

#include "Process/Types/MemoryRange.hpp"

using namespace Azoth;

TEST_CASE("MemoryRange basic validity and size")
{
    MemoryRange invalid;
    CHECK(invalid.empty());
    CHECK_FALSE(invalid.valid());
    CHECK(invalid.size() == 0u);
    CHECK(!invalid.contains(Address::null()));

    MemoryRange equalBounds{0x1000, 0x1000};
    CHECK(equalBounds.empty());
    CHECK_FALSE(equalBounds.valid());
    CHECK(equalBounds.size() == 0u);
}

TEST_CASE("MemoryRange constructor clamps reverse ranges")
{
    MemoryRange reversed{0x2000, 0x1000};
    CHECK(reversed.startAddr == Address{0x2000});
    CHECK(reversed.stopAddr == Address{0x2000});
    CHECK(reversed.empty());
    CHECK_FALSE(reversed.valid());
}

TEST_CASE("MemoryRange contains address and range membership")
{
    MemoryRange range{0x1000, 0x2000};

    CHECK(range.contains(Address{0x1000}));
    CHECK(range.contains(Address{0x1FFF}));
    CHECK_FALSE(range.contains(Address{0x2000}));
    CHECK_FALSE(range.contains(Address{0x0FFF}));

    MemoryRange inner{0x1200, 0x1F00};
    CHECK(range.contains(inner));

    MemoryRange overlapping{0x1800, 0x2800};
    CHECK_FALSE(range.contains(overlapping));

    MemoryRange outside{0x2000, 0x3000};
    CHECK_FALSE(range.contains(outside));
}

TEST_CASE("MemoryRange alignment helpers")
{
    MemoryRange range{0x1234, 0x4321};
    range.align(0x1000);

    CHECK(range.startAddr == Address{0x1000});
    CHECK(range.stopAddr == Address{0x5000});
    CHECK(range.isAligned(0x1000));
}

TEST_CASE("MemoryRange bounding and overlap")
{
    MemoryRange a{0x1000, 0x3000};
    MemoryRange b{0x2000, 0x4000};

    MemoryRange bounding = a.boundingRange(b);
    CHECK(bounding.startAddr == Address{0x1000});
    CHECK(bounding.stopAddr == Address{0x4000});

    MemoryRange overlap = a.overlapRange(b);
    CHECK(overlap.valid());
    CHECK(overlap.startAddr == Address{0x2000});
    CHECK(overlap.stopAddr == Address{0x3000});
    CHECK(overlap.size() == 0x1000u);

    MemoryRange disjointA{0x5000, 0x6000};
    MemoryRange disjointB{0x1000, 0x2000};
    MemoryRange noOverlap = disjointA.overlapRange(disjointB);
    CHECK(noOverlap.empty());
    CHECK_FALSE(noOverlap.valid());
}

TEST_CASE("MemoryRange relative and absolute conversions")
{
    MemoryRange range{0x1000, 0x2000};
    CHECK(range.toRelative(Address{0x1800}) == 0x800u);
    CHECK(range.toAbsolute(0x800u) == Address{0x1800});
}

TEST_CASE("MemoryRange string formatting")
{
    MemoryRange range{0x1000, 0x2000};
    const std::string rangeText = to_string(range);

    CHECK(rangeText.find("MemoryRange{ ") == 0u);
    CHECK(rangeText.find("start=0x0000000000001000") != std::string::npos);
    CHECK(rangeText.find("stop=0x0000000000002000") != std::string::npos);
    CHECK(rangeText.find("size=4096") != std::string::npos);

    std::ostringstream oss;
    oss << range;
    CHECK(oss.str() == rangeText);

    MemoryRange invalid;
    CHECK(to_string(invalid) == "MemoryRange{ invalid }");
}
