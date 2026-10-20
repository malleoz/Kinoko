#pragma once

#include <abstract/memory/List.hh>

namespace Kinoko::EGG {

class Heap;

/// @brief An interface for ensuring certain structures and classes are destroyed with the heap.
/// @details Singletons are expected to inherit Disposer.
class Disposer {
    /// @brief Grants access to @ref m_link so a @ref Host::Context can restore the linked list on
    /// context switch
    friend class Heap;

public:
    /// @brief Gets the offset of the link member within the @ref Disposer class
    /// @return The offset of the link member within the @ref Disposer class
    /// @details The C++ standard only guarantees `offsetof` for standard-layout types. Since @ref
    /// Disposer has a virtual destructor, `offset` of is not universally supported. Instead of
    /// hardcoding an offset, we derive it ourselves.
    [[nodiscard]] static constexpr u16 getLinkOffset() {
        return reinterpret_cast<uintptr_t>(&reinterpret_cast<Disposer *>(NULL)->m_link);
    }

protected:
    Disposer();
    virtual ~Disposer();

private:
    Heap *const m_heap;               ///< The heap to which this disposer belongs
    Abstract::Memory::MEMLink m_link; ///< This object's link in the heap's linked list of disposers
};

} // namespace Kinoko::EGG
