/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../ThirdParty/Zydis/Zydis.h"

#include <string>
#include <cstring>
#include <vector>
#include <optional>


namespace Azoth
{


class CProcess;

struct DecoderConfig
{

};

struct DecodedInstruction
{
	ZydisDecodedInstruction instr{}; //328 bytes
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{}; //80 Bytes * 10
	uint64_t runtimeAddress = 0;
};

struct CompactInstruction //40 Bytes
{
    uint64_t address;     // Instruction runtime address
	uint64_t attributes;  // ZydisInstructionAttributes
	//Consider compressing attribute

    uint16_t mnemonic;      // ZydisMnemonic (fits in 16-bit)
	uint8_t raw_bytes[15];  // Instruction bytes (up to 15 for x86/x64)
	//Consider 16 bytes since it gets padded anyways

    // Metadata packed in 32-bit
    struct Meta
    {
        uint32_t length        : 4;  // instruction length (max 15 bytes)
        uint32_t operand_count : 4;  // number of operands (max 10)
        uint32_t category      : 8;  // ZydisInstructionCategory (8 Bits)
        uint32_t placeholder   : 16; //
    } meta;

    // ----- Helpers -----

    // Get length of the instruction
    uint8_t length() const { return static_cast<uint8_t>(meta.length); }

    // Get number of operands
    uint8_t operands() const { return static_cast<uint8_t>(meta.operand_count); }

    // Get category
    ZydisInstructionCategory category() const { return static_cast<ZydisInstructionCategory>(meta.category); }

    // Check if attribute flag is set
    bool has_attribute(ZydisInstructionAttributes flag) const { return (attributes & flag) != 0; }

	bool is_nop() const { return mnemonic == ZYDIS_MNEMONIC_NOP; }

	bool is_call() const { return meta.category == ZYDIS_CATEGORY_CALL; }

	bool is_ret()  const { return meta.category == ZYDIS_CATEGORY_RET; }

	bool is_control_flow() const {
    	return meta.category == ZYDIS_CATEGORY_CALL || 
           	meta.category == ZYDIS_CATEGORY_COND_BR ||
           	meta.category == ZYDIS_CATEGORY_UNCOND_BR ||
           	meta.category == ZYDIS_CATEGORY_RET;
	}

	bool modifies_stack() const {
    	return meta.category == ZYDIS_CATEGORY_PUSH ||
        	meta.category == ZYDIS_CATEGORY_POP ||
           	meta.category == ZYDIS_CATEGORY_CALL ||
           	meta.category == ZYDIS_CATEGORY_RET;
	}

	bool is_terminator() const {
    	return is_control_flow() || mnemonic == ZYDIS_MNEMONIC_INT3 || mnemonic == ZYDIS_MNEMONIC_HLT;
	}

	bool is_relative() const {
		return attributes == ZYDIS_ATTRIB_IS_RELATIVE;
	}

    void fromZydis(const ZydisDecodedInstruction& instr, const uint8_t* bytes, uint64_t addr)
    {
        address = addr;
        mnemonic = static_cast<uint16_t>(instr.mnemonic);
        meta.length = instr.length;
        meta.operand_count = instr.operand_count;
        meta.category = instr.meta.category;
        attributes = instr.attributes;

        std::memcpy(raw_bytes, bytes, instr.length);
    }
};

class InstructionRangeIterator
{

};


class CDecoderModule
{
public:
    CDecoderModule(CProcess* backPtr);

public:
	CDecoderModule(const CDecoderModule&) = delete;

    CDecoderModule& operator=(const CDecoderModule&) = delete;

public:
    bool decodeAt(const uint8_t* buffer, size_t size, CompactInstruction& d, uint64_t runtimeAddr)
	{
		ZydisDecodedInstruction instr{};
		if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&_decoder, NULL, buffer, size, &instr)))
		{
			return false;
		}

		d.fromZydis(instr, buffer, runtimeAddr);
		return true;
	}

	bool decodeNext(const uint8_t*& buffer, size_t& size, uint64_t& runtimeAddr, CompactInstruction& out)
	{
		ZydisDecodedInstruction instr{};
    	if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&_decoder, nullptr, buffer, size, &instr)))
    	{
        	return false;
    	}

    	out.fromZydis(instr, buffer, runtimeAddr);

    	// Advance state
    	buffer      += instr.length;
    	size        -= instr.length;
    	runtimeAddr += instr.length;
    	return true;
	}

	//Consider Modern for loop iterators.
    InstructionRangeIterator range();

    std::vector<CompactInstruction> decodeRange(const uint8_t* buffer, size_t size, uint64_t baseAddr, const std::string& separator = "\n");


	uint64_t decodeAbsoluteMemoryAddress(const uint8_t* buffer, size_t bufferSize, uint64_t runtimeAddress, int operandIndex = -1)
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
				const auto& op = operands[i];
				if (op.type == ZYDIS_OPERAND_TYPE_MEMORY || op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
				{
					targetIndex = i;
					break;
				}
			}
		}

		if (targetIndex < 0 || targetIndex >= static_cast<int>(instr.operand_count))
			return 0;

		const auto& op = operands[targetIndex];
		if (op.type != ZYDIS_OPERAND_TYPE_MEMORY || op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
			return 0;

		uint64_t absAddr = 0;
		if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(&instr, &op, runtimeAddress, &absAddr)))
			return 0;

		return absAddr;
	}

    std::string formatInstruction(const CompactInstruction& instr) const
	{
		ZydisDecodedInstruction instruction;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

		if (ZYAN_FAILED(ZydisDecoderDecodeFull(&_decoder, instr.raw_bytes, sizeof(instr.raw_bytes), &instruction, operands)))
			return 0;

		char szBuffer[64];
		if (ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&_formatter, &instruction, operands, 
            instruction.operand_count, szBuffer, sizeof(szBuffer), instr.address, ZYAN_NULL)))
            {
                return std::string(szBuffer);
            }
		return "";
	}

    std::string formatRange(const std::vector<CompactInstruction>& instrs, const std::string& separator = "\n") const;

    InstructionRangeIterator createIterator(const uint8_t* buffer, size_t size, uint64_t baseAddr) const;

private:
	CProcess*      _backPtr; 
	ZydisDecoder   _decoder;   //20 Bytes
	ZydisFormatter _formatter; //600 Bytes
};


}