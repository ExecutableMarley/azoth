/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "CDecoderModule.hpp"

#include "../Types/EProcessArchitecture.hpp"
#include "../CProcess.hpp"

#include <mutex>

namespace Azoth
{

InstructionIterator& InstructionIterator::operator++()
{
    if (!_decoder || !_decoder->decodeNext(_buffer, _size, _addr, _current))
    {
        _decoder = nullptr; // mark end
        _buffer  = nullptr;
        _size    = 0;
        _addr    = 0;
    }
    return *this;
}

bool InstructionIterator::operator==(const InstructionIterator& other) const
{
    // End of range check
    if (_decoder == nullptr || other._decoder == nullptr)
	{
    	return _decoder == other._decoder;
	}
	// Actual iterator check
	return (_decoder == other._decoder) && (_buffer  == other._buffer) && 
    	(_addr == other._addr) && (_size == other._size);
}


//https://github.com/zyantific/zydis/blob/master/examples/Formatter01.c

ZydisFormatterFunc default_print_address_absolute;
ZyanStatus PrintAbsAddressHook(
    const ZydisFormatter* formatter,
    ZydisFormatterBuffer* buffer,
    ZydisFormatterContext* context)
{
    ZyanU64 address;
    ZYAN_CHECK(ZydisCalcAbsoluteAddress(context->instruction, context->operand,
        context->runtime_address, &address));
       
    //Check if user data exists
    if (context->user_data)
    {
        auto decoder = (CDecoderModule*)context->user_data;

        
    }

    //ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL)
    //ZydisFormatterBufferGetString(buffer, &string);
    //ZyanStringAppendFormat(string, "<%s>", SYMBOL_TABLE[i].name)
    //ZydisFormatter

    //return ZYAN_STATUS_ARG_MISSES_VALUE;
    return default_print_address_absolute(formatter, buffer, context);
}


CDecoderModule::CDecoderModule(CProcess* backPtr) : _backPtr(backPtr)
{
	EProcessArchitecture architecture = backPtr->GetArchitecture();

    if (sizeof(void*) == 4)
    {
        ZydisDecoderInit(&_decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
    }
    else
    {
        ZydisDecoderInit(&_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    }

	ZydisFormatterInit(&_formatter, ZYDIS_FORMATTER_STYLE_INTEL);

#if 1
    static std::mutex formatter_mutex;

    ZydisFormatterFunc absAddressHook = PrintAbsAddressHook;
    ZydisFormatterSetHook(&_formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void**)&absAddressHook);
    
    // Mutex locked block
    {
        std::scoped_lock lock(formatter_mutex);

        if (default_print_address_absolute == nullptr)
        {
            default_print_address_absolute = absAddressHook;
        }
    }
#endif

    _isReady = true;
}

void CDecoderModule::setTargetArchitecture(EProcessArchitecture architecture)
{
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
        return;
    }
}

std::ostream& CDecoderModule::formatInstruction(std::ostream& os, const CompactInstruction& instr) const
{
	if (!instr.isValid())
	{
		return os << "<invalid instruction>";
	}
	ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
	if (ZYAN_FAILED(ZydisDecoderDecodeFull(&_decoder, instr.raw_bytes, sizeof(instr.raw_bytes), &instruction, operands)))
	{
		return os << "<invalid instruction>";
	}
	char szBuffer[64];
	if (!ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&_formatter, &instruction, operands, instruction.operand_count, 
		szBuffer, sizeof(szBuffer), instr.address, ZYAN_NULL)))
    {
		return os << "<invalid instruction>";
	}
	return os << szBuffer;
}

std::string CDecoderModule::formatInstruction(const CompactInstruction& instr) const
{
	std::ostringstream oss;
	formatInstruction(oss, instr);
	return oss.str();
}



}