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
    if (!_decoder || !_decoder->decodeNext(_cursor, _current))
    {
        _decoder = nullptr; // mark end
        _cursor.buffer        = nullptr;
        _cursor.remainingSize = 0;
        _cursor.runtimeAddr   = 0;
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
	return (_decoder == other._decoder) && (_cursor.buffer  == other._cursor.buffer) && 
    	(_cursor.runtimeAddr == other._cursor.runtimeAddr) && (_cursor.remainingSize == other._cursor.remainingSize);
}

ZYAN_INLINE ZyanStatus ZydisStringAppendChar(ZyanString* destination, char c)
{
    ZYAN_ASSERT(destination);
    ZYAN_ASSERT(!destination->vector.allocator);
    ZYAN_ASSERT(destination->vector.size);

    if (destination->vector.size + 1 > destination->vector.capacity)
    {
        return ZYAN_STATUS_INSUFFICIENT_BUFFER_SIZE;
    }

    char* data = (char*)destination->vector.data;

    data[destination->vector.size - 1] = c;
    data[destination->vector.size] = '\0';

    destination->vector.size += 1;

    return ZYAN_STATUS_SUCCESS;
}

ZYAN_INLINE ZyanStatus ZydisStringAppendStringView(ZyanString* destination, std::string_view source)
{
    ZYAN_ASSERT(destination);
    ZYAN_ASSERT(!destination->vector.allocator);
    ZYAN_ASSERT(destination->vector.size);

    if (!source.size())
        return ZYAN_STATUS_SUCCESS;

    if (destination->vector.size + source.size() > destination->vector.capacity)
    {
        return ZYAN_STATUS_INSUFFICIENT_BUFFER_SIZE;
    }

    memcpy((char*)destination->vector.data + destination->vector.size - 1, source.data(), source.size());

    // copy terminating null
    ((char*)destination->vector.data)[destination->vector.size - 1 + source.size()] = '\0';

    destination->vector.size += source.size();

    return ZYAN_STATUS_SUCCESS;
}


ZYAN_INLINE ZyanStatus ZydisStringAppendHex(ZyanString* destination, uint64_t value, bool prefix0x)
{
    static const char HEX_LOOKUP[] = "0123456789ABCDEF";

    char buffer[18]; // 16 + '0x'
    int pos = 18;

    // Generate backwards
    do
    {
        buffer[--pos] = HEX_LOOKUP[value & 0xF];
        value >>= 4;
    } while (value);

    // Append prefix
    if (prefix0x)
    {
        buffer[--pos] = 'x';
        buffer[--pos] = '0';
    }

    return ZydisStringAppendStringView(destination, 
        std::string_view(buffer + pos, 18 - pos));
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

        ZYAN_CHECK(ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL));
        ZyanString* str;
        ZYAN_CHECK(ZydisFormatterBufferGetString(buffer, &str));

        ProcessImage image;
        uint64_t offset;
        ImageSymbol symbol;
        if (decoder->resolveSymbol(address, symbol))
        {
            std::string text;

            // Import Address Table entry (not direct function address)
            if (symbol.source == SymbolSource::Import)
            {
                text = "<&" + symbol.modName + "!" + symbol.name + ">";
            }
            // Normal symbol
            else
            {
                text = symbol.modName + "!" + symbol.name;
            }
            return ZydisStringAppendStringView(str, text);
        }
        else if (decoder->resolveModule(address, image, offset))
        {
            std::string text = image.name + "+" + std::to_string(offset);
            return ZydisStringAppendStringView(str, text);
        }
    }

    //ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL)
    //ZydisFormatterBufferGetString(buffer, &string);
    //ZyanStringAppendFormat(string, "<%s>", SYMBOL_TABLE[i].name)
    //ZydisFormatter

    //return ZYAN_STATUS_ARG_MISSES_VALUE;
    return default_print_address_absolute(formatter, buffer, context);
}

InstructionFormatter::InstructionFormatter(Style style)
{
    // 1. Set style and init Formatter
    ZydisFormatterStyle_ zydisStyle = ZYDIS_FORMATTER_STYLE_INTEL;
    if (style == Style::Intel)
        zydisStyle = ZYDIS_FORMATTER_STYLE_INTEL;
    if (style == Style::ATnT)
        zydisStyle = ZYDIS_FORMATTER_STYLE_ATT;
    if (style == Style::MASM)
        zydisStyle = ZYDIS_FORMATTER_STYLE_INTEL_MASM;

    ZydisFormatterInit(&_formatter, zydisStyle);

    // 2. Place Formatter Hook
    ZydisFormatterFunc absHook = PrintAbsAddressHook;
    ZydisFormatterSetHook(&_formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void**)&absHook);
    default_print_address_absolute = absHook;
}


CDecoderModule::CDecoderModule(CProcess* backPtr) : _backPtr(backPtr)
{
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

bool CDecoderModule::decodeAt(const uint8_t *buffer, size_t size, Address runtimeAddr, CompactInstruction &out)
{
    ZydisDecodedInstruction instr{};
    if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&_decoder, nullptr, buffer, size, &instr)))
    {
        return false;
    }

    out.fromZydis(instr, buffer, runtimeAddr);
    return true;
}

bool CDecoderModule::decodeAt(const uint8_t *buffer, size_t size, Address runtimeAddr, Instruction &out)
{
    if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&_decoder, &out.context, buffer, size, &out.instr)))
    {
        return false;
    }
    out.runtimeAddr = runtimeAddr;
    return true;
}

bool CDecoderModule::decodeNext(DecoderCursor &cursor, CompactInstruction &out)
{
    ZydisDecodedInstruction instr{};
    if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&_decoder, nullptr, cursor.buffer, cursor.remainingSize, &instr)))
    {
        return false;
    }
    out.fromZydis(instr, cursor.buffer, cursor.runtimeAddr);

    // Advance state
    cursor.advance(instr.length);
    return true;
}

bool CDecoderModule::decodeNext(DecoderCursor &cursor, Instruction &out)
{
    if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&_decoder, &out.context, cursor.buffer, cursor.remainingSize, &out.instr)))
    {
        return false;
    }
    out.runtimeAddr = cursor.runtimeAddr;

    // Advance state
    cursor.advance(out.instr.length);
    return true;
}

bool CDecoderModule::decodeOperands(const CompactInstruction &instr, InstructionOperands &out)
{
    ZydisDecodedInstruction zydisInstruction;
    if (ZYAN_FAILED(ZydisDecoderDecodeFull(&_decoder, instr.raw_bytes, sizeof(instr.raw_bytes), &zydisInstruction, out.operands)))
        return false;

    out.count = zydisInstruction.operand_count;
    return true;
}

bool CDecoderModule::decodeOperands(const Instruction &instr, InstructionOperands &out)
{
    if (ZYAN_FAILED(ZydisDecoderDecodeOperands(&_decoder, &instr.context, &instr.instr, out.operands, InstructionOperands::MaxOperands)))
        return false;

    out.count = instr.instr.operand_count;
    return true;
}

Address CDecoderModule::decodeAbsoluteMemoryAddress(const uint8_t *buffer, size_t bufferSize, Address runtimeAddress, int operandIndex)
{
    ZydisDecodedInstruction instr;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

    if (ZYAN_FAILED(ZydisDecoderDecodeFull(&_decoder, buffer, bufferSize, &instr, operands)))
        return 0;

    // Find operand index
    int targetIndex = operandIndex;
    if (targetIndex < 0)
    {
        for (uint32_t i = 0; i < instr.operand_count; ++i)
        {
            const auto &op = operands[i];
            if (op.type == ZYDIS_OPERAND_TYPE_MEMORY || op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            {
                targetIndex = i;
                break;
            }
        }
    }

    if (targetIndex < 0 || targetIndex >= static_cast<int>(instr.operand_count))
        return 0;

    const auto &op = operands[targetIndex];
    if (op.type != ZYDIS_OPERAND_TYPE_MEMORY || op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        return 0;

    uint64_t absAddr = 0;
    if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(&instr, &op, runtimeAddress, &absAddr)))
        return 0;

    return absAddr;
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
	char szBuffer[128];
	if (!ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&_formatter, &instruction, operands, instruction.operand_count, 
		szBuffer, sizeof(szBuffer), instr.address, (void*)this )))
    {
		return os << "<invalid instruction>";
	}
	return os << szBuffer;
}

std::ostream& CDecoderModule::formatInstruction(std::ostream& os, const Instruction& instr) const
{
	if (!instr.isValid())
	{
		return os << "<invalid instruction>";
	}
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
	if (ZYAN_FAILED(ZydisDecoderDecodeOperands(&_decoder, &instr.context, &instr.instr, operands, ZYDIS_MAX_OPERAND_COUNT)))
	{
		return os << "<invalid instruction>";
	}
	char szBuffer[128];
	if (!ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&_formatter, &instr.instr, operands, ZYDIS_MAX_OPERAND_COUNT, 
		szBuffer, sizeof(szBuffer), instr.runtimeAddr, (void*)this )))
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

std::string CDecoderModule::formatInstruction(const Instruction& instr) const
{
	std::ostringstream oss;
	formatInstruction(oss, instr);
	return oss.str();
}

bool CDecoderModule::resolveSymbol(Address runtimeAddress, ImageSymbol& outSymbol)
{
	return _backPtr->getSymbols().findSymbolByAddress(runtimeAddress, true, outSymbol);
}

bool CDecoderModule::resolveModule(Address runtimeAddress, ProcessImage& outImage, uint64_t& outOffset)
{
    if (_backPtr->getSymbols().findModuleByAddress(runtimeAddress, outImage))
    {
        outOffset = runtimeAddress - outImage.baseAddress;
        return true;
    }
    return false;
}

}