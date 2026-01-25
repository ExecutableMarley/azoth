#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest.h"

#include "Process/CProcess.hpp"


using namespace Azoth;

TEST_CASE("Compile Test")
{
    CProcess process(Platform::createDefaultLayer());

    REQUIRE(true);

    REQUIRE(process.initialize());
    REQUIRE(process.attach(Platform::getPID()));

    auto mainImage = process.getProcessMainImage();
    REQUIRE(mainImage.valid());

    process.getMemory();  //read,write,alloc,protect,free, enum regions
    process.getScanner(); //Pattern/Signature/Value Scan
}