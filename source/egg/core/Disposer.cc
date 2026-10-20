#include "Disposer.hh"

#include "egg/core/Heap.hh"

namespace Kinoko::EGG {

/// @addr{0x8021A0F0}
/// @brief Constructor
/// @details Initializes @ref m_heap via @ref Heap::FindContainHeap(). If the heap containing this
/// disposer was found, it appends this disposer to the heap's linked list via @ref
/// Heap::appendDisposer().
Disposer::Disposer() : m_heap(Heap::FindContainHeap(this)) {
    if (m_heap) {
        m_heap->appendDisposer(this);
    }
}

/// @addr{0x8021A144}
/// @brief Virtual destructor
/// @details Removes this disposer from the containing heap's linked list.
Disposer::~Disposer() {
    if (m_heap) {
        m_heap->removeDisposer(this);
    }
}

} // namespace Kinoko::EGG
