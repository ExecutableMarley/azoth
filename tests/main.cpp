#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest.h"

#include "Process/CProcess.hpp"

#include "Process/Platform/Windows/WinapiLink.hpp"

using namespace Azoth;

TEST_CASE("Compile Test")
{
    WinapiLink platformLink;
    CProcess process(&platformLink);

    REQUIRE(TRUE);

    REQUIRE(process.initialize());
    REQUIRE(process.attach(GetCurrentProcessId()));

    auto mainImage = process.getProcessMainImage();
    REQUIRE(mainImage.valid());

    process.getMemory();  //read,write,alloc,protect,free, enum regions
    process.getScanner(); //Pattern/Signature/Value Scan
}