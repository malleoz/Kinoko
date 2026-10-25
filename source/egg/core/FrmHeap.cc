#include "FrmHeap.hh"

namespace Kinoko::EGG {

using namespace Abstract::Memory;

/// @brief Public method to destroy this heap
/// @details Calls this heap's destructor and frees the heap's memory block from the parent heap.
void FrmHeap::destroy() {
    Heap *pParent = getParentHeap();
    this->~FrmHeap();
    if (pParent) {
        pParent->free(this);
    }
}

/// @brief Allocates a memory block of the provided size and alignment
/// @param size The size of the memory block to allocate
/// @param align The alignment requirement for the memory block
/// @return Pointer to the allocated memory block
void *FrmHeap::alloc(size_t size, s32 align) {
    return dynamicCastHandleToFrm()->alloc(size, align);
}

/// @brief No-op
/// @details Frame heaps can only free to the heap start or heap end depending on flags. Freeing by
/// block is invalid because the memory is not recovered.
void FrmHeap::free(void * /* block */) {}

/// @copydoc Heap::getAllocatableSize()
u32 FrmHeap::getAllocatableSize(s32 align) const {
    return dynamicCastHandleToFrm()->getAllocatableSize(align);
}

/// @brief Reinterpret casts @ref m_handle to @ref MEMiFrmHeapHead
/// @return Pointer to the memory heap handle as a @ref MEMiFrmHeapHead
MEMiFrmHeapHead *FrmHeap::dynamicCastHandleToFrm() {
    return reinterpret_cast<MEMiFrmHeapHead *>(m_handle);
}

/// @brief Reinterpret casts @ref m_handle to @ref MEMiFrmHeapHead
/// @return Const pointer to the memory heap handle as a @ref MEMiFrmHeapHead
const MEMiFrmHeapHead *FrmHeap::dynamicCastHandleToFrm() const {
    return reinterpret_cast<MEMiFrmHeapHead *>(m_handle);
}

/// @brief Creates a frame heap at the specified start address with the given size and options
/// @param startAddress The starting address of the memory block for the heap
/// @param size The size of the memory block for the heap
/// @param opt Options for the frame heap
/// @return Pointer to the created frame heap, or `nullptr` if creation failed
FrmHeap *FrmHeap::Create(void *startAddress, size_t size, u16 opt) {
    FrmHeap *heap = nullptr;

    void *endAddress = RoundDown(AddOffset(startAddress, size), 4);
    startAddress = RoundUp(startAddress, 4);

    size_t addrRange = GetAddrNum(endAddress) - GetAddrNum(startAddress);
    if (startAddress > endAddress || addrRange < sizeof(FrmHeap) + 4) {
        return nullptr;
    }

    void *handleStart = AddOffset(startAddress, sizeof(FrmHeap));
    MEMiFrmHeapHead *handle =
            MEMiFrmHeapHead::Create(handleStart, addrRange - sizeof(FrmHeap), opt);
    if (handle) {
        heap = new (startAddress) FrmHeap(handle);
    }

    return heap;
}

/// @brief Creates a frame heap with the specified size, parent heap, and options
/// @param size The size of the memory block for the heap
/// @param pHeap Pointer to the parent heap, or `nullptr` to use the current heap
/// @param opt Options for the frame heap
/// @return Pointer to the created frame heap, or `nullptr` if creation failed
/// @details If `size` is `std::numeric_limits<size_t>::max()`, the heap will attempt to allocate
/// the maximum possible size from the parent heap.
FrmHeap *FrmHeap::Create(size_t size, Heap *pHeap, u16 opt) {
    FrmHeap *heap = nullptr;

    if (!pHeap) {
        pHeap = Heap::GetCurrentHeap();
    }

    if (size == std::numeric_limits<size_t>::max()) {
        size = pHeap->getAllocatableSize();
    }

    void *block = pHeap->alloc(size, 4);
    if (block) {
        heap = Create(block, size, opt);
        if (heap) {
            heap->setParentHeap(pHeap);
        } else {
            pHeap->free(block);
        }
    }

    return heap;
}

/// @brief Private constructor
/// @param handle The handle for the memory heap
FrmHeap::FrmHeap(MEMiHeapHead *handle) : Heap(handle) {}

/// @brief Private destructor
/// @details Destroys all children @ref Disposer objects via @ref dispose() and destroys @ref
/// m_handle.
FrmHeap::~FrmHeap() {
    dispose();
    dynamicCastHandleToFrm()->destroy();
}

} // namespace Kinoko::EGG
