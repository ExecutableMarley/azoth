#include "../../third_party/doctest.h"

#include "Process/Types/Pattern.hpp"

using namespace Azoth;

TEST_CASE("Pattern default empty state")
{
    Pattern pattern;
    CHECK(pattern.empty());
    CHECK(pattern.size() == 0u);
    CHECK(pattern.toString().empty());
}

TEST_CASE("Pattern construction from bytes and mask")
{
    std::vector<uint8_t> bytes{0xDE, 0xAD, 0xBE, 0xEF};
    std::vector<uint8_t> mask{0xFF, 0x00, 0xFF, 0xFF};
    Pattern pattern(bytes, mask);

    CHECK(!pattern.empty());
    CHECK(pattern.size() == 4u);
    CHECK_FALSE(pattern.isWildcard(0));
    CHECK(pattern.isWildcard(1));
    CHECK_FALSE(pattern.isWildcard(2));
    CHECK_FALSE(pattern.isWildcard(3));

    const BYTE data[] = {0xDE, 0x12, 0xBE, 0xEF};
    CHECK(pattern.matches(data, 4u));
    CHECK_FALSE(pattern.matches(data, 3u));
}

TEST_CASE("Pattern combo string parsing")
{
    Pattern pattern("48 8B ? ? 89");

    CHECK(pattern.size() == 5u);
    CHECK(pattern.bytes[0] == 0x48u);
    CHECK(pattern.bytes[1] == 0x8Bu);
    CHECK(pattern.isWildcard(2));
    CHECK(pattern.isWildcard(3));
    CHECK_FALSE(pattern.isWildcard(4));

    const BYTE data[] = {0x48, 0x8B, 0xFE, 0xED, 0x89};
    CHECK(pattern.matches(data, 5u));

    const BYTE mismatch[] = {0x48, 0x8C, 0xFE, 0xED, 0x89};
    CHECK_FALSE(pattern.matches(mismatch, 5u));
}

TEST_CASE("Pattern combo parsing is flexible with optional 0x prefix and extra whitespace")
{
    Pattern pattern("0x48  8b ??  89");

    CHECK(pattern.size() == 4u);
    CHECK(pattern.bytes[0] == 0x48u);
    CHECK(pattern.bytes[1] == 0x8Bu);
    CHECK(pattern.isWildcard(2));
    CHECK(pattern.bytes[3] == 0x89u);
}

TEST_CASE("Pattern construction from raw data and mask string")
{
    const BYTE data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    Pattern pattern(data, "x??x");

    CHECK(pattern.size() == 4u);
    CHECK_FALSE(pattern.isWildcard(0));
    CHECK(pattern.isWildcard(1));
    CHECK(pattern.isWildcard(2));
    CHECK_FALSE(pattern.isWildcard(3));
    CHECK(pattern.matches(data, 4u));
}

TEST_CASE("Pattern trimTrailingWildcards removes wildcard suffix")
{
    Pattern pattern("AA BB ? ?");
    CHECK(pattern.size() == 4u);

    pattern.trimTrailingWildcards();
    CHECK(pattern.size() == 2u);
    CHECK(pattern.toString() == "AA BB");
}

TEST_CASE("Pattern invalid input results in empty pattern")
{
    Pattern invalidPattern("48 8B GG");
    CHECK(invalidPattern.empty());
    CHECK(invalidPattern.size() == 0u);
}

TEST_CASE("Pattern text formatting and stream insertion")
{
    Pattern pattern("AA ?? BB");
    const std::string stringified = pattern.toString();
    CHECK(stringified == "AA ?? BB");

    std::ostringstream oss;
    oss << pattern;
    CHECK(oss.str() == stringified);
}
