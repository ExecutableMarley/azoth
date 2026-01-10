#pragma once

#include "../ThirdParty/Zydis/Zydis.h"

#include <string>
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

struct CompactInstruction
{
	uint64_t address;
	uint8_t  length;

	ZydisMnemonic mnemonic;

	uint8_t operandCount;

	ZydisInstructionAttributes attributes;

	uint8_t bytes[15];
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
    bool decodeAt(const uint8_t* buffer, size_t size, DecodedInstruction& d, uint64_t runtimeAddr);

    DecodedInstruction decodeAt(const uint8_t* buffer, size_t size, uint64_t runtimeAddr);

    std::vector<DecodedInstruction> decodeRange(const uint8_t* buffer, size_t size, uint64_t baseAddr, const std::string& separator = "\n");


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

    std::string formatInstruction(const DecodedInstruction& instr) const;

    std::string formatRange(const std::vector<DecodedInstruction>& instrs, const std::string& separator = "\n") const;

    InstructionRangeIterator createIterator(const uint8_t* buffer, size_t size, uint64_t baseAddr) const;

private:
	CProcess*      _backPtr; 
	ZydisDecoder   _decoder;   //20 Bytes
	ZydisFormatter _formatter; //600 Bytes
};


}