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
#include <ostream>
#include <sstream>

namespace Azoth
{


class CProcess;
class CDecoderModule;

struct DecoderConfig
{

};

struct DecodedInstruction
{
	ZydisDecodedInstruction instr{}; //328 bytes
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{}; //80 Bytes * 10
	uint64_t runtimeAddress = 0;
};

/**
 * @brief Compact representation of a decoded x86/x64 instruction.
 *
 * Size: 40 bytes (padded on 64-bit platforms)
 */
struct CompactInstruction
{
	/** @brief Runtime address of the instruction (RIP/EIP at decode time). */
    uint64_t address;     // Instruction runtime address
	
	uint64_t attributes;  // ZydisInstructionAttributes
	//Consider compressing attribute

	/**
     * @brief Instruction mnemonic (ZydisMnemonic).
     *
     * ZYDIS_MNEMONIC_INVALID indicates an uninitialized or invalid instruction.
     */
    uint16_t mnemonic = ZYDIS_MNEMONIC_INVALID;

	/**
     * Raw instruction bytes.
	 * 
     * Only the first @ref meta.length bytes are valid.
	 * 
	 * @note x86/x64 instructions are at most 15 bytes long.
     */
	uint8_t raw_bytes[15];
	//Consider 16 bytes since it gets padded anyways

    // Metadata packed in 32-bit
    struct Meta
    {
		/** Instruction length in bytes (1–15). */
        uint32_t length        : 4;

		/** Number of explicit operands (0–10). */
        uint32_t operand_count : 4;

		/** Instruction category (ZydisInstructionCategory). */
        uint32_t category      : 8;

		/** Reserved */
        uint32_t placeholder   : 16; //
    } meta;

    // ----- Helpers -----

	/**
     * @brief Check whether this instruction contains valid decoded data.
     */
	bool isValid() const { return mnemonic != ZYDIS_MNEMONIC_INVALID; }

    /** @brief Get the length of the instruction in bytes. */
    uint8_t length() const { return static_cast<uint8_t>(meta.length); }

    /** @brief Get the number of explicit operands. */
    uint8_t operands() const { return static_cast<uint8_t>(meta.operand_count); }

    /** @brief Get the instruction category. */
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
		return (attributes & ZYDIS_ATTRIB_IS_RELATIVE) != 0;
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

class InstructionOperands
{
public:
	static constexpr size_t MaxOperands = 10;

	//Consider wrap around helper class
	ZydisDecodedOperand operands[MaxOperands];

	size_t count = 0;

	void reset() { count = 0; }

	ZydisDecodedOperand& operator[](size_t index)
	{
		return operands[index];
	}
};

class InstructionIterator
{
public:
    using value_type = CompactInstruction;
    using reference  = const CompactInstruction&;
    using pointer    = const CompactInstruction*;
    using iterator_category = std::input_iterator_tag;

    InstructionIterator() = default;

    InstructionIterator(CDecoderModule* decoder, const uint8_t* buffer, size_t size, uint64_t addr)
        : _decoder(decoder), _buffer(buffer), _size(size), _addr(addr)
    {
        ++(*this); // decode first
    }

    reference operator*() const { return _current; }

    pointer   operator->() const { return &_current; }

    InstructionIterator& operator++();

    bool operator==(const InstructionIterator& other) const;

    bool operator!=(const InstructionIterator& other) const
    {
        return !(*this == other);
    }

private:
    CDecoderModule* _decoder = nullptr;
    const uint8_t*  _buffer  = nullptr;
    size_t          _size    = 0;
    uint64_t        _addr    = 0;
    CompactInstruction _current{};
};

class InstructionRange
{
public:
    InstructionRange(CDecoderModule* decoder, const uint8_t* buffer, size_t size, uint64_t addr)
        : _decoder(decoder), _buffer(buffer), _size(size), _addr(addr) {}

    InstructionIterator begin()
    {
        return InstructionIterator(_decoder, _buffer, _size, _addr);
    }

    InstructionIterator end()
    {
        return InstructionIterator();
    }

private:
    CDecoderModule* _decoder;
    const uint8_t*  _buffer;
    size_t          _size;
    uint64_t        _addr;
};


class CDecoderModule
{
	struct FormatterProxy {
        const CDecoderModule& module;
        const CompactInstruction& ci;

		//Proxy operator
        friend std::ostream& operator<<(std::ostream& os, const FormatterProxy& proxy) {
            return proxy.module.formatInstruction(os, proxy.ci);
        }
    };
public:
    CDecoderModule(CProcess* backPtr);

public:
	CDecoderModule(const CDecoderModule&) = delete;

    CDecoderModule& operator=(const CDecoderModule&) = delete;

public:
    bool decodeAt(const uint8_t* buffer, size_t size, uint64_t runtimeAddr, CompactInstruction& out)
	{
		ZydisDecodedInstruction instr{};
		if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&_decoder, nullptr, buffer, size, &instr)))
		{
			return false;
		}

		out.fromZydis(instr, buffer, runtimeAddr);
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

	//Iterator support
    InstructionRange range(const uint8_t* buffer, size_t size, uint64_t addr)
	{
		return InstructionRange(this, buffer, size, addr);
	}

	bool decodeOperands(const CompactInstruction& instr, InstructionOperands& out)
	{
		ZydisDecodedInstruction zydisInstruction;
		if (ZYAN_FAILED(ZydisDecoderDecodeFull(&_decoder, instr.raw_bytes, sizeof(instr.raw_bytes), &zydisInstruction, out.operands)))
			return false;

		return true;
	}

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

	std::ostream& formatInstruction(std::ostream& os, const CompactInstruction& instr) const;

    std::string formatInstruction(const CompactInstruction& instr) const;

	//Slightly more convenient << use
	FormatterProxy wrap(const CompactInstruction& ci) const {
        return { *this, ci };
    }

private:
	CProcess*      _backPtr; 
	ZydisDecoder   _decoder;   //20 Bytes
	ZydisFormatter _formatter; //600 Bytes
};


}