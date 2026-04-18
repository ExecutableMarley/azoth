#include "../../third_party/doctest.h"

#include "Process/Types/Address.hpp"

using namespace Azoth;

TEST_CASE("Address basics")
{
    Address address;

    CHECK(address == Address::null());
    CHECK(address == Address::invalid());
    CHECK(address.raw() == 0);
    CHECK_FALSE(address.isValid());
    CHECK(address.isAligned(1));

    Address nonZero{0x1234};
    CHECK(nonZero != address);
    CHECK(nonZero == 0x1234u);
    CHECK(nonZero.raw() == 0x1234u);
    CHECK(nonZero.isValid());

    CHECK(Address::minAddr().raw() == 0x10000u);
    CHECK(Address::maxAddr32().raw() == 0xFFFFFFFFull);
    CHECK(Address::maxAddr().raw() == 0x7FFFFFFFFFFFull);
}

TEST_CASE("Address arithmetic and comparisons")
{
    Address start{0x10};
    Address moved = start + 0x5u;
    CHECK(moved == Address{0x15});
    CHECK((start + 0x0u) == start);

    Address subtracted = moved - 0x5u;
    CHECK(subtracted == start);

    start += 0x10u;
    CHECK(start == Address{0x20});
    CHECK((Address{0x20} - Address{0x10}) == 0x10u);

    CHECK(Address{0x10} < Address{0x20});
    CHECK(Address{0x20} > Address{0x10});
    CHECK(Address{0x10} < 0x20u);
    CHECK(Address{0x20} > 0x10u);
}

TEST_CASE("Address alignment helpers")
{
    Address x{0x21};

    CHECK(x.alignDown(0x10) == Address{0x20});
    CHECK(x.alignUp(0x10) == Address{0x30});
    CHECK(Address{0x20}.isAligned(0x10));
    CHECK_FALSE(Address{0x21}.isAligned(0x10));

    const char* ptr = reinterpret_cast<const char*>(0x12345u);
    CHECK(Address::alignDown(ptr, 0x1000) == reinterpret_cast<const char*>(0x12000u));
    CHECK(Address::alignUp(ptr, 0x1000) == reinterpret_cast<const char*>(0x13000u));
    CHECK(Address::isAligned(reinterpret_cast<const char*>(0x12000u), 0x1000));
    CHECK_FALSE(Address::isAligned(reinterpret_cast<const char*>(0x12345u), 0x1000));
}

TEST_CASE("Address pointer conversion and string formatting")
{
    int value = 42;
    const void* rawPtr = &value;
    Address address = Address::fromPtr(rawPtr);

    CHECK(address.raw() == reinterpret_cast<uint64_t>(rawPtr));
    CHECK(address.as<const void>() == rawPtr);

    const std::string stringified = to_string(address);
    CHECK(stringified.rfind("0x", 0) == 0);
    CHECK(stringified.length() == sizeof(Address) * 2 + 2);

    std::ostringstream oss;
    oss << address;
    CHECK(oss.str() == stringified);
}
