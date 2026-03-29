/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include "../ThirdParty/Zydis/Zydis.h"
#include "../Types/Address.hpp"
#include "../Types/EProcessArchitecture.hpp"
#include "../Core/ProcessImage.hpp"

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



/**
 * @brief Compact representation of a decoded x86/x64 instruction.
 *
 * @note obsolete
 * 
 * Size: 40 bytes (padded on 64-bit platforms)
 */
struct CompactInstruction
{
	/** @brief Runtime address of the instruction (RIP/EIP at decode time). */
    Address address;
	
	// ZydisInstructionAttributes
	uint64_t attributes;
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

/**
 * @brief Represents a single decoded machine instruction.
 *
 **/
class Instruction
{
public:
	/**
	 * @brief Returns the length of the instruction in bytes.
	 */
	uint8_t length() const { return _instr.length; }

	/**
	 * @brief Returns the runtime address of the instruction.
	 */
	Address addr() const { return _runtimeAddr; }

	/**
	 * @brief Returns the address immediately after the instruction.
	 */
	Address end()  const { return _runtimeAddr + _instr.length; }

	/**
	 * @brief Returns the address of the next sequential instruction.
	 */
	Address next() const { return end(); }

	/**
	 * @brief Checks whether the given address lies within this instruction.
	 */
	bool contains(Address addr) const { return addr >= _runtimeAddr && addr < end(); }

	/**
	 * @brief Returns the underlying decoded ZydisDecodedInstruction object.
	 */
	const ZydisDecodedInstruction& raw() const noexcept
	{
    	return _instr;
	}

	/**
     * @brief Check whether this instruction contains valid decoded data.
     */
	bool isValid() const { return _instr.mnemonic != ZYDIS_MNEMONIC_INVALID; }

	/**
	 * @brief Checks whether the instruction is a NOP.
	 */
	bool isNop() const { return _instr.mnemonic == ZYDIS_MNEMONIC_NOP; }

	/**
	 * @brief Checks whether the instruction is a call.
	 */
	bool isCall() const { return _instr.meta.category == ZYDIS_CATEGORY_CALL; }

	/**
	 * @brief Checks whether the instruction is a conditional or unconditional jump.
	 */
	bool isJump() const
	{
        return _instr.meta.category == ZYDIS_CATEGORY_COND_BR ||
               _instr.meta.category == ZYDIS_CATEGORY_UNCOND_BR;
    }

	/**
	 * @brief Checks whether the instruction is a return.
	 */
	bool isRet() const { return _instr.meta.category == ZYDIS_CATEGORY_RET; }
	
	/**
	 * @brief Checks whether the instruction transfers control flow via branch or call.
	 */
	bool isBranch() const { return isCall() || isJump(); }

	/**
	 * @brief Checks whether the instruction affects control flow.
	 */
	bool isControlFlow() const
	{
		return _instr.meta.category == ZYDIS_CATEGORY_CALL || 
           	_instr.meta.category == ZYDIS_CATEGORY_COND_BR ||
           	_instr.meta.category == ZYDIS_CATEGORY_UNCOND_BR ||
           	_instr.meta.category == ZYDIS_CATEGORY_RET;
	}

	/** 
	 * @brief Checks if the instruction is an unconditional jump or a return. 
     * @note These instructions usually mark the end of a basic block with no fall-through.
     */
	bool isUnconditionalFlow() const
    {
        return _instr.meta.category == ZYDIS_CATEGORY_UNCOND_BR || 
               _instr.meta.category == ZYDIS_CATEGORY_RET;
    }

	/**
	 * @brief Checks whether this instruction terminates linear execution.
	 *
	 * Includes control-flow instructions as well as trap or halt instructions.
	 */
	bool isTerminator() const
	{
		return isControlFlow() || _instr.mnemonic == ZYDIS_MNEMONIC_INT3 || _instr.mnemonic == ZYDIS_MNEMONIC_HLT;
	}

	/**
	 * @brief Checks whether the instruction modifies the stack pointer.
	 */
	bool modifiesStack() const
	{
		return _instr.meta.category == ZYDIS_CATEGORY_PUSH ||
        	_instr.meta.category == ZYDIS_CATEGORY_POP ||
           	_instr.meta.category == ZYDIS_CATEGORY_CALL ||
           	_instr.meta.category == ZYDIS_CATEGORY_RET;
	}

	/**
	 * @brief Checks whether the instruction uses relative addressing.
	 */
	bool isRelative() const { return (_instr.attributes & ZYDIS_ATTRIB_IS_RELATIVE) != 0; }

	/**
	 * @brief Checks whether the instruction contains an immediate operand.
	 */
	bool hasImmediate() const { return _instr.raw.imm[0].size != 0 || _instr.raw.imm[1].size != 0; }

	/**
	 * @brief Checks whether the instruction contains a displacement.
	 */
	bool hasDisplacement() const { return _instr.raw.disp.size != 0; }

	/**
	 * @brief Checks whether the instruction contains an immediate or displacement operand.
	 */
	bool hasImmediateOrDisplacement() const { return hasImmediate() || hasDisplacement(); }

	//Consider if RIP-relative addressing is covered
	/**
	 * @brief Checks whether the instruction may reference a runtime address.
	 *
	 * This is typically the case for instructions using relative immediate
	 * or displacements that resolve to code or data addresses.
	 */
	bool mayReferenceAddress() const { return isRelative() && hasImmediateOrDisplacement(); }

	/**
	 * @brief Computes the absolute address referenced by an operand.
	 *
	 * @param operand Operand to resolve.
	 * @param outAddr Receives the computed address on success.
	 * @return True if an address could be resolved.
	 */
	bool getAbsoluteAddress(const ZydisDecodedOperand& operand, uint64_t& outAddr) const
	{
		if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY && operand.type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
        	return false;
    	}

		return ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&_instr, &operand, _runtimeAddr, &outAddr));
	}

	/**
	 * @brief Computes the absolute address referenced by an operand.
	 *
	 * @param operand Operand to resolve.
	 * 
	 * Returns a null address if the operand cannot be resolved.
	 */
	Address getAbsoluteAddress(const ZydisDecodedOperand& operand) const
	{
		if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY && operand.type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
        	return Address::null();
    	}

		uint64_t outAddr;
		if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&_instr, &operand, _runtimeAddr, &outAddr)))
			return outAddr;
		return Address::null();
	}

private:
	Address _runtimeAddr = 0;
	ZydisDecodedInstruction _instr{};
	ZydisDecoderContext _context{};

	friend class CDecoderModule;
	friend class InstructionFormatter;
};

/**
 * @brief Container for decoded instruction operands.
 */
class InstructionOperands
{
public:
	/** @brief Maximum number of operands supported by the decoder. */
	static constexpr size_t MaxOperands = 10;

	/**
	 * @brief Clears all stored operands.
	 */
	void reset() { count = 0; }

	/**
	 * @brief Returns the number of operands.
	 */
	size_t size() const { return count; }

	/**
	 * @brief Checks whether no operands are present.
	 */
	bool empty() const { return count == 0; }

	/**
	 * @brief Returns an iterator to the first operand.
	 */
	auto begin() { return operands; }

	/**
	 * @brief Returns an iterator past the last operand.
	 */
	auto end()   { return operands + count; }

	/**
	 * @brief Returns an iterator to the first operand.
	 */
	auto begin() const { return operands; }

	/**
	 * @brief Returns an iterator past the last operand.
	 */
	auto end() const   { return operands + count; }

	/**
	 * @brief Returns the operand at the given index.
	 *
	 * @param index Operand index.
	 */
	ZydisDecodedOperand& operator[](size_t index)
	{
		assert(index < count);
		return operands[index];
	}

private:
	size_t count = 0;

	//Consider wrap around helper class
	ZydisDecodedOperand operands[MaxOperands];

	friend class CDecoderModule;
};

/**
 * @brief Cursor used to iterate through a buffer during instruction decoding.
 * 
 * The cursor tracks the current buffer position, the number of remaining bytes,
 * and the corresponding runtime address. It is typically advanced as instructions
 * are decoded sequentially.
 */
class DecoderCursor
{
public:
	const uint8_t* buffer;
	size_t remainingSize;
	uint64_t runtimeAddr;

	/**
	 * @brief Returns the runtime address of the current position.
	 */
	Address currentAddress() const { return runtimeAddr; }

	/**
	 * @brief Returns a pointer to the end of the buffer.
	 */
	const uint8_t* end() const { return buffer + remainingSize; }

	/**
	 * @brief Checks whether more bytes remain to be processed.
	 */
	bool hasMore() const { return remainingSize > 0; }

	/**
	 * @brief Checks whether the cursor has reached the end of the buffer.
	 */
	bool empty()   const { return remainingSize == 0; }

	/**
	 * @brief Checks whether at least @p size bytes remain.
	 */
	bool canRead(size_t size) const { return remainingSize >= size; }

	/**
	 * @brief Advances the cursor by the specified number of bytes.
	 *
	 * If fewer bytes remain than requested, the cursor advances to the end.
	 */
	void advance(size_t size)
	{
		size_t actualStep = std::min(size, remainingSize);

		buffer        += actualStep;
    	remainingSize -= actualStep;
    	runtimeAddr   += actualStep;
	}

	/**
	 * @brief Checks whether the cursor still contains data.
	 */
	explicit operator bool() const noexcept { return remainingSize > 0; }
};

class InstructionIterator
{
public:
    using value_type = Instruction;
    using reference  = const Instruction&;
    using pointer    = const Instruction*;
    using iterator_category = std::input_iterator_tag;

    InstructionIterator() = default;

    InstructionIterator(CDecoderModule* decoder, const uint8_t* buffer, size_t size, uint64_t addr)
        : _decoder(decoder), _cursor{ buffer, size, addr }
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
    DecoderCursor   _cursor{};
    Instruction _current{};
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

/**
 * @brief Currently not used
 */
class InstructionFormatter
{
public:
	enum class Style
	{
		Intel,
		ATnT,
		MASM
	};

    std::ostream& format(std::ostream& os, const Instruction& instr) const;

	class FormattedInstruction
	{
	public:
		FormattedInstruction(const Instruction &instr,
							 const InstructionFormatter &formatter)
			: _instr(instr), _formatter(formatter) {}

		friend std::ostream &operator<<(std::ostream &os, const FormattedInstruction &f)
		{
			return f._formatter.format(os, f._instr);
		}

	private:
		const Instruction &_instr;
		const InstructionFormatter &_formatter;
	};

	static const InstructionFormatter& get(Style style = Style::Intel)
	{
		static InstructionFormatter intel(Style::Intel);
		static InstructionFormatter att(Style::ATnT);
		static InstructionFormatter masm(Style::MASM);

		switch (style)
    	{
        	case Style::Intel: return intel;
        	case Style::ATnT:  return att;
        	case Style::MASM:  return masm;
    	}

		return intel;
	}

private:
	ZydisFormatter _formatter;

	InstructionFormatter(Style style);
};


/**
 * @brief Instruction decoder for a target process architecture.
 * 
 *  Provides functionality to decode raw instruction bytes into structured
 *  representations, iterate over instruction streams, resolve operands and
 *  memory addresses, and format instructions for output.
 */
class CDecoderModule
{
public:
    CDecoderModule(CProcess* backPtr);

public:
	CDecoderModule(const CDecoderModule&) = delete;

    CDecoderModule& operator=(const CDecoderModule&) = delete;

public:

	void setTargetArchitecture(EProcessArchitecture architecture);

	//setFormatting style

	//Todo: Reconsider CompactInstruction class
	//Maybe we should simply add an c++ wrapper around ZydisDecodedInstruction instead
	//As long as we do not want to store the instructions. Size does not matter anyways

    bool decodeAt(const uint8_t* buffer, size_t size, Address runtimeAddr, CompactInstruction& out);
	
	bool decodeAt(const uint8_t* buffer, size_t size, Address runtimeAddr, Instruction& out);

	bool decodeNext(DecoderCursor& cursor, CompactInstruction& out);

	bool decodeNext(DecoderCursor& cursor, Instruction& out);

	//Iterator support
    InstructionRange range(const uint8_t* buffer, size_t size, uint64_t addr)
	{
		return InstructionRange(this, buffer, size, addr);
	}

	bool decodeOperands(const CompactInstruction& instr, InstructionOperands& out);

	bool decodeOperands(const Instruction& instr, InstructionOperands& out);

	Address decodeAbsoluteMemoryAddress(const uint8_t* buffer, size_t bufferSize, Address runtimeAddress, int operandIndex = -1);

	std::ostream& formatInstruction(std::ostream& os, const CompactInstruction& instr) const;

	std::ostream& formatInstruction(std::ostream& os, const Instruction& instr) const;

    std::string formatInstruction(const CompactInstruction& instr) const;

	std::string formatInstruction(const Instruction& instr) const;

private:
	template <typename T>
	struct FormatterProxy {
        const CDecoderModule& module;
        const T& ci;

		//Proxy operator
        friend std::ostream& operator<<(std::ostream& os, const FormatterProxy& proxy) {
            return proxy.module.formatInstruction(os, proxy.ci);
        }
    };

public:
	//Slightly more convenient << use
	template <typename T>
	FormatterProxy<T> fmt(const T& ci) const
	{
        return { *this, ci };
    }

	bool resolveSymbol(Address runtimeAddress, ImageSymbol& outSymbol);

	bool resolveModule(Address runtimeAddress, ProcessImage& outImage, uint64_t& outOffset);

private:
	CProcess*      _backPtr; 
	ZydisDecoder   _decoder;   //20 Bytes
	ZydisFormatter _formatter; //600 Bytes
	bool _isReady = false;
};


}