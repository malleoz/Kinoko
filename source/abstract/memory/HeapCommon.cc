#include "HeapCommon.hh"

#include <cstring>

namespace Kinoko::Abstract::Memory {

/// @brief Gets the list of all child heaps
/// @return The list of all child heaps, @ref m_childList
MEMList &MEMiHeapHead::getChildList() {
    return m_childList;
}

/// @brief Gets a pointer to the start of this heap
/// @return A pointer to the start of this heap, @ref m_heapStart
void *MEMiHeapHead::getHeapStart() {
    return m_heapStart;
}

/// @brief Gets a pointer to the end of this heap
/// @return A pointer to the end of this heap, @ref m_heapEnd
void *MEMiHeapHead::getHeapEnd() {
    return m_heapEnd;
}

/// @brief Gets the global list of all root heaps
/// @return The list of all root heaps, @ref s_rootList
MEMList &MEMiHeapHead::GetRootList() {
    return s_rootList;
}

/// @addr{0x80198658}
/// @brief Finds the heap that contains the specified memory block, starting from the root heap
/// @param block A pointer to the memory block
/// @return A pointer to the heap handler for the heap containing the memory block, or `nullptr` if
/// not found
MEMiHeapHead *MEMiHeapHead::FindContainHeap(const void *block) {
    return FindContainHeap(&s_rootList, block);
}

/// @addr{0x8019832C}
/// @brief Private constructor
/// @param signature The signature of the heap
/// @param heapStart A pointer to the start of the heap
/// @param heapEnd A pointer to the end of the heap
/// @param opt The options for the heap
/// @details Sets @ref m_signature to `signature, @ref m_optFlag to `opt`, @ref m_heapStart to
/// `heapStart`, and @ref m_heapEnd to `heapEnd`. Depending on @ref m_optFlag, calls @ref
/// fillNoUseMemory to initialize all bytes in the heap to the appropriate fill value. For release
/// builds, this is a no-op to improve performance. Finally, appends the heap to its containing
/// list, or to @ref s_rootList if this is a root heap.
MEMiHeapHead::MEMiHeapHead(u32 signature, void *heapStart, void *heapEnd, const OptFlag &opt)
    : m_signature(signature),
      m_optFlag(opt),
      m_childList(MEMList(getLinkOffset())),
      m_heapStart(heapStart),
      m_heapEnd(heapEnd) {
    fillNoUseMemory(heapStart, GetAddrNum(heapEnd) - GetAddrNum(heapStart));

    findListContainHeap().append(this);
}

/// @addr{0x801984EC}
/// @brief Private destructor
/// @details Removes the heap from its containing list, or from @ref s_rootList if this is a root
/// heap.
MEMiHeapHead::~MEMiHeapHead() {
    findListContainHeap().remove(this);
}

/// @brief Initializes all bytes in an unallocated heap block
/// @param address The address of the heap block to fill
/// @param size The number of bytes to initialize
/// @details This is a no-op by default for performance reasons. If @ref eOptFlag::DebugFillAlloc is
/// set in @ref m_optFlag, then the memory is initialized with a `0xC3C3C3C3` byte pattern.
void MEMiHeapHead::fillNoUseMemory(void *address, u32 size) {
    if (m_optFlag.onBit(eOptFlag::DebugFillAlloc)) {
        memset(address, GetFillVal(FillType::NoUse), size);
    }
}

/// @brief Initializes all bytes in a newly allocated heap block
/// @param address The address of the heap block to fill
/// @param size The number of bytes to initialize
/// @details This is a no-op by default for performance reasons. If @ref eOptFlag::DebugFillAlloc is
/// set in @ref m_optFlag, then the memory is initialized with a `0xF3F3F3F3` byte pattern. If @ref
/// eOptFlag::ZeroFillAlloc is set, the memory is zero-initialized.
void MEMiHeapHead::fillAllocMemory(void *address, u32 size) {
    if (m_optFlag.onBit(eOptFlag::DebugFillAlloc)) {
        memset(address, GetFillVal(FillType::Alloc), size);
    } else if (m_optFlag.onBit(eOptFlag::ZeroFillAlloc)) {
        memset(address, 0, size);
    }
}

/// @brief Initializes all bytes in a newly freed heap block
/// @param address The address of the heap block to fill
/// @param size The number of bytes to initialize
/// @details This is a no-op by default for performance reasons. If @ref eOptFlag::DebugFillAlloc is
/// set in @ref m_optFlag, then the memory is initialized with a `0xD3D3D3D3` byte pattern.
void MEMiHeapHead::fillFreeMemory(void *address, u32 size) {
    if (m_optFlag.onBit(eOptFlag::DebugFillAlloc)) {
        memset(address, GetFillVal(FillType::Free), size);
    }
}

/// @addr{0x801981EC}
/// @brief Recursive function to find the heap that contains the specified memory block
/// @param list The list of heaps to search within
/// @param block The memory block to locate
/// @return Pointer to the handler for the heap that contains the specified memory block, or
/// `nullptr` if not found
/// @details Recursively searches through the provided list of heaps and their child heaps by
/// comparing their address bounds (@ref m_heapStart and @ref m_heapEnd) to locate the heap that
/// contains the specified memory block.
MEMiHeapHead *MEMiHeapHead::FindContainHeap(MEMList *list, const void *block) {
    MEMiHeapHead *heap = nullptr;

    while ((heap = reinterpret_cast<MEMiHeapHead *>(list->getNext(heap)))) {
        if (GetAddrNum(heap->m_heapStart) > GetAddrNum(block) ||
                GetAddrNum(block) >= GetAddrNum(heap->m_heapEnd)) {
            continue;
        }

        MEMiHeapHead *search = FindContainHeap(&heap->m_childList, block);
        if (search) {
            return search;
        }

        return heap;
    }

    return nullptr;
}

/// @brief Gets the child heap list for the current heap
/// @return Reference to the current heap's child heap list, or the root list if not found
MEMList &MEMiHeapHead::findListContainHeap() const {
    MEMiHeapHead *containHeap = FindContainHeap(this);
    return containHeap ? containHeap->getChildList() : GetRootList();
}

MEMList MEMiHeapHead::s_rootList = MEMList(MEMiHeapHead::getLinkOffset());

} // namespace Kinoko::Abstract::Memory
