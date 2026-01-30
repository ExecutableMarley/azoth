/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <chrono>
#include <cassert>

#include "../Types/Address.hpp"

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

	Address get(CMemoryModule& mem, std::chrono::milliseconds cacheDuration);

	bool hasChildren()    const noexcept { return _childCount > 0; }
	uint32_t childCount() const noexcept { return _childCount; }

private:
	bool update(CMemoryModule& mem, std::chrono::milliseconds cacheDuration);

	void addChild()    noexcept { ++_childCount; }
    void removeChild() noexcept { --_childCount; }

private:
	PtrChainLink*  _parent = 0;
	uint64_t       _addr   = 0;
	Address        _ptr    = 0;
	clock::time_point _lastUpdate{};
	uint32_t   _childCount = 0;
};


/**
 * @class PointerEndpoint
 * @brief Represents the end address of a pointer chain or a fixed absolute address.
 * 
 * Complex endpoints can only be constructed by @ref CMemoryModule, which owns and manages
 * the underlying @ref PtrChainLink nodes and their caching behavior.
 */
class PointerEndpoint
{
public:
	PointerEndpoint() noexcept = default;

	/**
	 * @brief Constructs a simple endpoint representing an absolute address.
	 *
	 * This constructor creates a non-complex endpoint that does not perform any
	 * pointer follow logic.
	 *
	 * @param absoluteAddr Absolute address in the target address space.
	 */
	PointerEndpoint(uint64_t absoluteAddr) noexcept
		: _addOffset(absoluteAddr) {}

protected:
	PointerEndpoint(CMemoryModule* mem, PtrChainLink* previous, uint64_t addOffset = 0) noexcept
		: _mem(mem), _previous(previous), _addOffset(addOffset)
		{

		}

public:

	/**
	 * @brief Checks whether this endpoint represents a usable address.
	 *
	 * @return True if the endpoint is either bound to a pointer chain or represents
	 *         a non-zero absolute address.
	 */
	bool valid() const
	{
		//_previous node should also return a non 0 result
		return this->_previous || this->_addOffset;
	}

	/**
	 * @brief Checks whether this endpoint is backed by a pointer chain.
	 *
	 * @return True if the endpoint resolves its address via pointer dereferencing,
	 *         false if it represents a simple absolute address.
	 */
	bool isComplex() const
	{
		return this->_previous != nullptr;
	}

	/**
	 * @brief Resolves the final address using a custom cache duration.
	 *
	 * @param[in] cacheDuration Maximum allowed age of cached pointer values.
	 * @return Resolved final address.
	 */
	Address get(std::chrono::milliseconds cacheDuration) const;

	/**
	 * @brief Resolves the final address using the default cache duration.
	 *
	 * The cache duration configured in the owning @ref CMemoryModule is used.
	 *
	 * @return Resolved final address.
	 */
	Address get() const;

	/**
	 * @brief Implicit conversion to the resolved address.
	 *
	 * Equivalent to calling @ref get() with the default cache duration.
	 */
	operator uint64_t() const
	{
		return this->get();
	}

private:
	CMemoryModule* _mem      = nullptr;
	PtrChainLink*  _previous = nullptr;
	uint64_t       _addOffset = 0;
};


}