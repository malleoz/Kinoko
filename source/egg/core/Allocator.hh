#pragma once

#include "egg/core/Heap.hh"

namespace Kinoko::EGG {

/// @brief Custom allocator for EGG framework using egg_alloc and egg_free
/// @param T The type of objects that this allocator will manage
/// @details This struct exists to allow for standard library containers to use the custom EGG
/// allocation functions, such as @ref alloc_list.
template <typename T>
struct Allocator {
    /// @brief The type of objects that this allocator will manage
    /// @details Defined in order to meet the C++ standard named requirements for an Allocator
    using value_type = T;

    /// @brief Default constructor
    Allocator() = default;

    /// @brief Converting constructor
    /// @tparam U The value type of the source allocator
    /// @details A container like `std::list` may need to allocate `Node<T>` to manage its elements
    /// internally. This constructor allows this to be possible.
    template <typename U>
    constexpr Allocator(const Allocator<U> &) noexcept {}

    /// @brief Allocates `n` count of objects of type `T`
    /// @param n The number of objects to allocate
    /// @return Pointer to the allocated memory for `n` objects of type `T`
    [[nodiscard]] T *allocate(size_t n) {
        return static_cast<T *>(egg_alloc(n * sizeof(T), static_cast<s32>(alignof(T))));
    }

    /// @brief Deallocates memory previously allocated for objects of type `T`
    /// @param ptr Pointer to the memory to deallocate
    void deallocate(T *ptr, size_t) noexcept {
        egg_free(ptr);
    }
};

/// @brief Equality operator
/// @return Always `true` so all instances of this allocator are considered equal
template <typename T, typename U>
constexpr bool operator==(const Allocator<T> &, const Allocator<U> &) noexcept {
    return true;
}

} // namespace Kinoko::EGG
