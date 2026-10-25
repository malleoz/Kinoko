#include "egg/core/ExpHeap.hh"

using namespace Kinoko;
using namespace Abstract::Memory;

namespace Kinoko::EGG {

/// @addr{0x802296E8}
/// @brief Constructor
/// @param handle The handle for the memory heap
/// @details Sets @ref m_handle to the provided `handle`. Initializes @ref m_children with the link
/// offset computed via @ref Disposer::getLinkOffset(). Initializes @ref m_parentHeap to `nullptr`.
/// Sets @ref m_name to `NoName` and clears all flags in @ref m_flags. Finally, appends this heap to
/// the global heap list, @ref s_heapList.
Heap::Heap(MEMiHeapHead *handle) : m_handle(handle), m_children(Disposer::getLinkOffset()) {
    m_parentHeap = nullptr;
    m_name = "NoName";
    m_flags.makeAllZero();

    s_heapList.append(this);
}

/// @addr{0x80229780}
/// @brief Virtual destructor that removes this heap from the global heap list, @ref s_heapList
Heap::~Heap() {
    s_heapList.remove(this);
}

/// @addr{0x80229C5C}
/// @brief Destroys all children @ref Disposer objects managed by this heap
/// @details Loops so long as @ref m_children is not empty. Each destructor call (@ref
/// Disposer::~Disposer) removes the disposer from the list of children.
void Heap::dispose() {
    Disposer *node = nullptr;
    while ((node = reinterpret_cast<Disposer *>(m_children.getFirst()))) {
        node->~Disposer();
    }

    ASSERT(!m_children.m_headObject && !m_children.m_tailObject);
}

/// @brief Disables allocation for this heap
/// @details Sets the @ref eFlags::Lock flag in @ref m_flags.
void Heap::disableAllocation() {
    m_flags.setBit(eFlags::Lock);
}

/// @brief Enables allocation for this heap
/// @details Clears the @ref eFlags::Lock flag in @ref m_flags.
void Heap::enableAllocation() {
    m_flags.resetBit(eFlags::Lock);
}

/// @brief Checks if allocation is currently disabled for this heap
/// @return `true` if allocation is disabled, `false` otherwise
bool Heap::tstDisableAllocation() const {
    return m_flags.onBit(eFlags::Lock);
}

/// @brief Appends a @ref Disposer object to the list of managed children
/// @param disposer The disposer to append to the list of managed children
void Heap::appendDisposer(Disposer *disposer) {
    m_children.append(disposer);
}

/// @brief Removes the provided @ref Disposer object from the list of managed chilren
/// @param disposer The disposer to remove from the list of managed children
void Heap::removeDisposer(Disposer *disposer) {
    m_children.remove(disposer);
}

/// @addr{0x80229D74}
/// @brief Sets this heap as the current heap and returns the previous current heap
/// @return The previous value of @ref s_currentHeap
Heap *Heap::becomeCurrentHeap() {
    Heap *oldHeap = s_currentHeap;
    s_currentHeap = this;
    return oldHeap;
}

/// @brief Retrieves the start address of this heap
/// @return A pointer to the start of this heap
/// @details The start of the heap is simply the address of `thisptr`.
void *Heap::getStartAddress() {
    return this;
}

/// @brief Retrieves the end address of this heap
/// @return A pointer to the end of this heap
/// @details Retrieved via calling @ref Abstract::Memory::MEMiHeapHead::getHeapEnd() on @ref
/// m_handle.
void *Heap::getEndAddress() {
    return m_handle->getHeapEnd();
}

/// @brief Retrieves the name of this heap
/// @return The name of this heap, @ref m_name
const char *Heap::getName() const {
    return m_name;
}

/// @addr{0x80229AD4}
/// @brief Retrieves the parent of this heap
/// @return The parent heap of this heap, or `nullptr` if it has no parent
Heap *Heap::getParentHeap() const {
    return m_parentHeap;
}

/// @brief Sets the name of this heap
/// @param name The name to set for this heap
void Heap::setName(const char *name) {
    m_name = name;
}

/// @brief Sets the parent heap for this heap
/// @param heap The parent heap to set
void Heap::setParentHeap(Heap *heap) {
    m_parentHeap = heap;
}

/// @addr{0x80229814}
/// @brief Allocates a block of memory from the specified heap, or the current heap if none is
/// specified.
/// @param size The size of the memory block to allocate
/// @param align The alignment requirement for the memory block
/// @param pHeap The heap from which to allocate the memory block, or `nullptr` to use the current
/// heap
/// @return A pointer to the allocated memory block, or `nullptr` if allocation fails
/// @details If @ref s_allocatableHeap is not `nullptr`, the allocation will only succeed if the
/// specified heap (`pHeap`) is equal to @ref s_allocatableHeap (or if `pHeap` is `nullptr`, then
/// @ref s_currentHeap must be equal to @ref s_allocatableHeap). Otherwise, a warning will be
/// emitted and the function will return `nullptr`.
/// @warning This function will emit a warning log and return `nullptr` if any of the following
/// scenarios occur:
/// 1. @ref s_allocatableHeap is not `nullptr`, `pHeap` is `nullptr`, @ref s_currentHeap is not
///    `nullptr`, and @ref s_currentHeap is not equal to @ref s_allocatableHeap
/// 2. @ref s_allocatableHeap is not `nullptr`, `pHeap` is not `nullptr`, and `pHeap` is not equal
///    to @ref s_allocatableHeap
/// 3. `pHeap` is `nullptr`, @ref s_currentHeap is not `nullptr`, and the allocation to @ref
///    s_currentHeap failed (either due to size limitations or the heap being locked)
/// 4. `pHeap` is `nullptr` and @ref s_currentHeap is `nullptr`
void *Heap::Alloc(size_t size, int align, Heap *pHeap) {
    Heap *currentHeap = s_currentHeap;

    if (s_allocatableHeap) {
        if (currentHeap && !pHeap) {
            pHeap = currentHeap;
        }

        if (pHeap != s_allocatableHeap) {
            WARN("HEAP ALLOC FAIL (%p, %s): Allocatable heap is %p (%s)", pHeap, pHeap->getName(),
                    s_allocatableHeap, s_allocatableHeap->getName());

            return nullptr;
        }
    }

    if (pHeap) {
        return pHeap->alloc(size, align);
    }

    if (currentHeap) {
        void *block = currentHeap->alloc(size, align);

        if (!block) {
            u32 heapFreeSize = currentHeap->getAllocatableSize(0x4);
            s32 heapSize = GetAddrNum(currentHeap->getEndAddress()) -
                    GetAddrNum(currentHeap->getStartAddress());

            constexpr f32 BYTES_TO_MBYTES = 1024.0f * 1024.0f;
            f32 heapSizeMB = static_cast<f32>(heapSize) / BYTES_TO_MBYTES;
            f32 heapFreeSizeMB = static_cast<f32>(heapFreeSize) / BYTES_TO_MBYTES;
            f32 sizeMB = static_cast<f32>(size) / BYTES_TO_MBYTES;

            WARN("HEAP ALLOC FAIL (%p, %s):\nTotal bytes: %d (%.1fMBytes)\nFree bytes: %d "
                 "(%.1fMBytes)\nAlloc bytes: %zu "
                 "(%.1fMBytes)\nAlign: %d",
                    currentHeap, currentHeap->getName(), heapSize, static_cast<f64>(heapSizeMB),
                    heapFreeSize, static_cast<f64>(heapFreeSizeMB), size, static_cast<f64>(sizeMB),
                    align);
        }

        return block;
    }

    WARN("HEAP ALLOC FAIL: Cannot allocate %zu from heap %p", size, pHeap);
    return nullptr;
}

/// @addr{0x80229B84}
/// @brief Frees the provided memory block from the provided heap
/// @param block The memory block to free
/// @param pHeap The heap from which to free the memory block, or `nullptr` to automatically
/// determine the heap containing the block
/// @warning If `pHeap` is `nullptr` and the heap containing the block cannot be determined, the
/// memory block will not be freed.
void Heap::Free(void *block, Heap *pHeap) {
    // Deleting nullptr should be no-op
    if (!block) {
        return;
    }

    if (!pHeap) {
        MEMiHeapHead *handle = MEMiHeapHead::FindContainHeap(block);
        if (!handle) {
            return;
        }

        pHeap = FindHeap(handle);
        if (!pHeap) {
            return;
        }
    }

    pHeap->free(block);
}

/// @brief Finds the heap corresponding to the provided handle
/// @param handle The @ref MEMiHeapHead handle to check
/// @return A pointer to the heap corresponding to the provided handle, or `nullptr` if no such heap
/// exists
Heap *Heap::FindHeap(MEMiHeapHead *handle) {
    Heap *node = nullptr;
    while ((node = reinterpret_cast<Heap *>(s_heapList.getNext(node)))) {
        if (node->m_handle == handle) {
            return node;
        }
    }

    return nullptr;
}

/// @addr{0x80229ADC}
/// @brief Finds the heap that contains the specified memory block
/// @param block The memory block to check
/// @return A pointer to the heap that contains the memory block, or `nullptr` if no such heap
/// exists
/// @details Finds the @ref Abstract::Memory::MEMiHeapHead containing the memory block and then
/// calls @ref FindHeap() to get the corresponding @ref Heap pointer.
Heap *Heap::FindContainHeap(const void *block) {
    MEMiHeapHead *handle = MEMiHeapHead::FindContainHeap(block);
    return handle ? FindHeap(handle) : nullptr;
}

/// @brief Reinterpret casts the provided @ref Heap to @ref ExpHeap, if safe
/// @param heap The heap to reinterpret cast
/// @return A pointer to the reinterpreted @ref ExpHeap if the cast is safe, otherwise `nullptr`
ExpHeap *Heap::DynamicCastToExp(Heap *heap) {
    return heap->getHeapKind() == Kind::Expanded ? reinterpret_cast<ExpHeap *>(heap) : nullptr;
}

/// @brief Reinterpret casts the provided @ref Heap to @ref FrmHeap, if safe
/// @param heap The heap to reinterpret cast
/// @return A pointer to the reinterpreted @ref FrmHeap if the cast is safe, otherwise `nullptr`
FrmHeap *Heap::DynamicCastToFrm(Heap *heap) {
    return heap->getHeapKind() == Kind::Frame ? reinterpret_cast<FrmHeap *>(heap) : nullptr;
}

/// @brief Gets the currently active heap
/// @return A pointer to the currently active heap, @ref s_currentHeap
Heap *Heap::GetCurrentHeap() {
    return s_currentHeap;
}

/// @brief Allocates a block of memory from the specified heap
/// @param size The size of the memory block to allocate
/// @param align The alignment requirement for the memory block
/// @param pHeap The heap from which to allocate the memory block
/// @return A pointer to the allocated memory block
void *egg_alloc(size_t size, s32 align, Heap *pHeap) {
    return Heap::Alloc(size, align, pHeap);
}

/// @brief Frees a previously allocated block of memory from the specified heap
/// @param block The memory block to free
/// @param pHeap The heap from which to free the memory block
void egg_free(void *block, Heap *pHeap) {
    Heap::Free(block, pHeap);
}

MEMList Heap::s_heapList = MEMList(Heap::getOffset());
Heap *Heap::s_currentHeap = nullptr;
Heap *Heap::s_allocatableHeap = nullptr;

} // namespace Kinoko::EGG
