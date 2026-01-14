/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#include "CDecoderModule.hpp"

#include "../Types/EProcessArchitecture.hpp"
#include "../CProcess.hpp"

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