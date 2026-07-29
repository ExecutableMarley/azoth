


/**
 * Prevents the compiler from optimizing away these variables during 
 * dead-code elimination, ensuring they remain in memory for scanning.
 */

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_ReadWriteBarrier)
#endif

template<typename T>
inline void anchor(T const& value)
{
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(&value) : "memory");
#elif defined(_MSC_VER)
    // Force a side-effect
    auto volatile* force_read = reinterpret_cast<const volatile char*>(&value);
    (void)force_read; 
    _ReadWriteBarrier();
#else
    static_cast<void>(value); // Fallback
#endif
}