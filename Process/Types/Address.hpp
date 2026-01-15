/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <iomanip>

namespace Azoth
{


/**
 * @brief Strongly-typed wrapper for a virtual memory address.
 *
 * No assumptions are made about the validity or accessibility of the address.
 */
class Address
{
public:
	using value_type = uint64_t;

private:
	value_type value = 0;

public:
    /** @brief Constructs a null (0) address. */
    constexpr Address() noexcept = default;

    /** @brief Implicitly constructs an Address from a raw unsigned 64-bit integer. */
    constexpr Address(value_type v) noexcept : value(v) {}

    /** @brief Explicitly constructs an Address from a local pointer. */
    explicit Address(const void* ptr) noexcept
	    : value(reinterpret_cast<value_type>(ptr)) {}

    /** @brief Implicit conversion to the underlying 64-bit integer. */
    constexpr operator value_type() const noexcept { return value; }

    /** @brief Explicitly converts to a local pointer. */
    explicit operator void*() const noexcept
    {
	    return reinterpret_cast<void*>(value);
    }

    /** @brief Returns true if the address is non-zero. */
    constexpr bool isValid() const noexcept { return value != 0; }

    /** @brief Casts the address to a local pointer of type T*. */
    template <typename T>
    constexpr T* as() const noexcept
    {
	    return reinterpret_cast<T*>(value);
    }

    constexpr value_type raw() const noexcept { return value; }

    /** @brief Returns a new Address aligned down to the nearest multiple of 'align'.
     *  @param align Must be a power of two.
     */
    constexpr Address alignDown(value_type align) const noexcept
    {
	    return Address(value & ~(align - 1));
    }

    /** @brief Returns a new Address aligned up to the nearest multiple of 'align'.
     *  @param align Must be a power of two.
     */
    constexpr Address alignUp(value_type align) const noexcept
    {
	    return Address((value + align - 1) & ~(align - 1));
    }

    constexpr Address isAligned(value_type align) const noexcept
    {
        return value == (value & ~(align-1));
    }

    // --- Comparison Operators ---

    constexpr bool operator==(const Address& other) const noexcept { return value == other.value; }
    constexpr bool operator!=(const Address& other) const noexcept { return value != other.value; }
    constexpr bool operator< (const Address& other) const noexcept { return value < other.value; }
    constexpr bool operator> (const Address& other) const noexcept { return value > other.value; }
    
    // --- Arithmetic Operators ---
    
    constexpr Address operator+  (value_type offset) const noexcept { return Address(value + offset); }
    constexpr Address operator-  (value_type offset) const noexcept { return Address(value - offset); }
    constexpr Address& operator+=(value_type offset) noexcept       { value += offset; return *this; }
    constexpr Address& operator-=(value_type offset) noexcept       { value -= offset; return *this; }

    constexpr value_type operator-(const Address& other) const noexcept { return value - other.value; }

    // --- Static Factory ---

    /** @brief Minimum safe user-mode address (64KB). */
    static constexpr Address minAddr()   { return 0x10000; }

    /** @brief Maximum address for 32-bit (4GB). */
    static constexpr Address maxAddr32() { return 0xFFFFFFFF; }

    /** @brief Maximum user-mode address for 64-bit (128TB). */
    static constexpr Address maxAddr()   { return 0x7FFFFFFFFFFF; }

    // --- Static Factories ---

    /** @brief Returns a null (zero) address. */
    static constexpr Address null() noexcept
    {
        return Address{};
    }

    // --- Static Helpers ---

    template <typename T>
    static inline T* alignDown(T* ptr, size_t align) noexcept
    {
        uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
        uintptr_t aligned_addr = (addr & ~(align -1));
        return reinterpret_cast<T*>(aligned_addr);
    }

    template <typename T>
    static inline T* alignUp(T* ptr, size_t align) noexcept
    {
        uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
        uintptr_t aligned_addr = ((addr + align - 1) & ~(align - 1));
        return reinterpret_cast<T*>(aligned_addr);
    }

    template <typename T>
    static inline bool isAligned(T* ptr, size_t align) noexcept
    {
        return (reinterpret_cast<std::uintptr_t>(ptr) & (align - 1)) == 0;
    }
};

/**
 * @brief Stream insertion operator for Address objects.
 *
 * @param os    Output stream.
 * @param addr  Memory Address to print.
 * @return Reference to the output stream.
 */
inline std::ostream& operator<<(std::ostream& os, const Address& addr)
{
    const auto oldFlags = os.flags();
    const auto oldFill  = os.fill();

    os << "0x"
       << std::hex
       << std::setw(sizeof(Address) * 2)
       << std::setfill('0')
       << static_cast<uint64_t>(addr);

    os.flags(oldFlags);
    os.fill(oldFill);
    return os;
}

/**
 * @brief Convert a Address to a string.
 *
 * This is a convenience wrapper around the stream insertion operator.
 *
 * @param addr Address to convert.
 * @return String representation of the Address.
 */
inline std::string to_string(const Address& addr)
{
    std::ostringstream oss;
    oss << addr;
    return oss.str();
}


} // namespace Azoth