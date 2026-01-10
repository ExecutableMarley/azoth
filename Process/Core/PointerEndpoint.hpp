/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>


namespace Azoth
{


class CMemoryModule;

class PtrChainLink
{
public:
	PtrChainLink(CMemoryModule* mem, uint64_t addr)
	{
		this->_mem = _mem;
		this->_parent = 0;
		this->_addr = addr;
		this->_ptr = 0;
		//Timer
	}

	PtrChainLink(CMemoryModule* mem, PtrChainLink* parent, uint64_t addr)
	{
		this->_mem = mem;
		this->_parent = parent;
		this->_addr = addr;
		this->_ptr = 0;
		//this->_timer = CTimer();
		//this->update();
	}

	PtrChainLink* getParent() const
	{
		return this->_parent;
	}

	uint64_t getAddr() const
	{
		return this->_addr;
	}

	void update();

	uint64_t get();

private:
	CMemoryModule* _mem;
	PtrChainLink*  _parent;
	uint64_t       _addr;
	uint64_t       _ptr;
	//CTimer        _timer;
};


/**
 * @class PointerEndpoint
 * @brief Represents the end of a pointer chain in memory.
 */
class PointerEndpoint
{
public:
	PointerEndpoint()
	{
		this->_previous = 0;
		this->_addOffset = 0;
	}

	PointerEndpoint(uint64_t addOffset)
	{
		this->_previous = 0;
		this->_addOffset = addOffset;
	}

	PointerEndpoint(PtrChainLink* previous, uint64_t addOffset = 0)
	{
		this->_previous = previous;
		this->_addOffset = addOffset;
	}

	bool valid() const
	{
		return this->_previous || this->_addOffset;
	}

	bool isComplex() const
	{
		return this->_previous != nullptr;
	}

	uint64_t get() const;

	operator uint64_t() const
	{
		return this->get();
	}

private:
	PtrChainLink* _previous;
	uint64_t      _addOffset;
};


}