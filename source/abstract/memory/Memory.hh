#pragma once

#include <Common.hh>

namespace Kinoko::Abstract::Memory {

/// @brief Retrieves the numeric address of a pointer
/// @param p The pointer to convert
/// @return The numeric address corresponding to the pointer
[[nodiscard]] static inline uintptr_t GetAddrNum(const void *p) {
    return reinterpret_cast<uintptr_t>(p);
}

/// @brief Converts a numeric address to a pointer
/// @param n The numeric address to convert
/// @return The pointer corresponding to the numeric address
[[nodiscard]] static inline void *GetAddrPtr(uintptr_t n) {
    return reinterpret_cast<void *>(n);
}

/// @brief Adds the specified offset to the address pointed to by `p`
/// @param p The pointer to apply an offset to
/// @param offset The offset to apply
/// @return The pointer resulting from adding the offset to `p`
[[nodiscard]] static inline void *AddOffset(const void *p, size_t offset) {
    return GetAddrPtr(GetAddrNum(p) + offset);
}

/// @brief Subtracts the specified offset from the address pointed to by `p`
/// @param p The pointer to apply an offset to
/// @param offset The offset to subtract
/// @return The pointer resulting from subtracting the offset from `p`
[[nodiscard]] static inline void *SubOffset(const void *p, size_t offset) {
    return GetAddrPtr(GetAddrNum(p) - offset);
}

/// @brief Rounds up the given value to the nearest multiple of the specified alignment
/// @param value The value to round up
/// @param alignment The alignment to round up to
/// @return The value rounded up to the nearest multiple of the alignment
[[nodiscard]] static inline uintptr_t RoundUp(uintptr_t value, uintptr_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

/// @brief Rounds up the given pointer to the nearest multiple of the specified alignment
/// @param ptr The pointer to round up
/// @param alignment The alignment to round up to
/// @return The pointer rounded up to the nearest multiple of the alignment
[[nodiscard]] static inline void *RoundUp(void *ptr, uintptr_t alignment) {
    return GetAddrPtr(RoundUp(GetAddrNum(ptr), alignment));
}

/// @brief Rounds down the given value to the nearest multiple of the specified alignment
/// @param value The value to round down
/// @param alignment The alignment to round down to
/// @return The value rounded down to the nearest multiple of the alignment
[[nodiscard]] static inline uintptr_t RoundDown(uintptr_t value, uintptr_t alignment) {
    return value & ~(alignment - 1);
}

/// @brief Rounds down the given pointer to the nearest multiple of the specified alignment
/// @param ptr The pointer to round down
/// @param alignment The alignment to round down to
/// @return The pointer rounded down to the nearest multiple of the alignment
[[nodiscard]] static inline void *RoundDown(void *ptr, uintptr_t alignment) {
    return GetAddrPtr(RoundDown(GetAddrNum(ptr), alignment));
}

} // namespace Kinoko::Abstract::Memory
