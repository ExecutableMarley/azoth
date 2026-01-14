/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <chrono>
#include <cassert>

namespace Azoth
{


class CMemoryModule;

class PtrChainLink
{
	friend class CMemoryModule;
public:
	using clock = std::chrono::steady_clock;

	PtrChainLink(const PtrChainLink&) = delete;
	PtrChainLink& operator=(const PtrChainLink&) = delete;
	PtrChainLink(PtrChainLink&&) = delete;
	PtrChainLink& operator=(PtrChainLink&&) = delete;

protected:
	PtrChainLink(uint64_t addr) noexcept
        : _parent(nullptr), _addr(addr) {}

    PtrChainLink(PtrChainLink* parent, uint64_t addr) noexcept
        : _parent(parent), _addr(addr)
		{
			if (_parent)
            	_parent->addChild();
		}

	~PtrChainLink()
    {
		assert(_childCount == 0 && "Destroying node with active children");
        if (_parent)
            _parent->removeChild();
    }

public:
	PtrChainLink* parent() const noexcept { return _parent; }
	uint64_t      offset() const noexcept { return _addr; }

	uint64_t get(CMemoryModule& mem, std::chrono::milliseconds cacheDuration);

	bool hasChildren()    const noexcept { return _childCount > 0; }
	uint32_t childCount() const noexcept { return _childCount; }

private:
	void update(CMemoryModule& mem, std::chrono::milliseconds cacheDuration);

	void addChild()    noexcept { ++_childCount; }
    void removeChild() noexcept { --_childCount; }

private:
	PtrChainLink*  _parent = 0;
	uint64_t       _addr   = 0;
	uint64_t       _ptr    = 0;
	clock::time_point _lastUpdate{};
	uint32_t   _childCount = 0;
};


/**
 * @class PointerEndpoint
 * @brief Represents the end of a pointer chain in memory.
 */
class PointerEndpoint
{
public:
	PointerEndpoint() noexcept = default;

	PointerEndpoint(uint64_t absoluteAddr) noexcept
		: _addOffset(absoluteAddr) {}

protected:
	PointerEndpoint(CMemoryModule* mem, PtrChainLink* previous, uint64_t addOffset = 0) noexcept
		: _mem(mem), _previous(previous), _addOffset(addOffset)
		{

		}

public:
	bool valid() const
	{
		return this->_previous || this->_addOffset;
	}

	bool isComplex() const
	{
		return this->_previous != nullptr;
	}

	uint64_t get(std::chrono::milliseconds cacheDuration) const;

	uint64_t get() const;

	operator uint64_t() const
	{
		return this->get();
	}

private:
	CMemoryModule* _mem      = nullptr;
	PtrChainLink*  _previous = nullptr;
	uint64_t      _addOffset = 0;
};


}