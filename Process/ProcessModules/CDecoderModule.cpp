#include "CDecoderModule.hpp"

#include "../Types/EProcessArchitecture.hpp"
#include "../CProcess.hpp"

namespace Azoth
{

//Todo: Handle this better
CDecoderModule::CDecoderModule(CProcess* backPtr) : _backPtr(backPtr)
{
	EProcessArchitecture architecture = backPtr->GetArchitecture();

    if (architecture == EProcessArchitecture::ARM32 || architecture == EProcessArchitecture::ARM64)
    {
        //Not supported. 
        return;
    }

	if (architecture == EProcessArchitecture::x86)
	{
		ZydisDecoderInit(&_decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
	}
	else if (architecture == EProcessArchitecture::x64)
	{
		ZydisDecoderInit(&_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
	}
    else
    {
        if constexpr (sizeof(void*) == 8)
        {
            ZydisDecoderInit(&_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        }
        else if constexpr (sizeof(void*) == 4)
        {
            ZydisDecoderInit(&_decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
        }
    }

	ZydisFormatterInit(&_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
}


}