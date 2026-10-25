#include "FrameHeap.hh"

#include <cstdlib>

namespace Kinoko::Abstract::Memory {

/// @brief Destroys the heap handle
void MEMiFrmHeapHead::destroy() {
    this->~MEMiFrmHeapHead();
}

/// @addr{0x801994E4}
/// @brief Allocates a memory block from the heap
/// @param size The size of the memory block to allocate
/// @param align The alignment of the block. Positive values allocate from the head of the heap,
/// negative values allocate from the tail.
/// @return A pointer to the allocated block, or `nullptr` if there is not enough space
void *MEMiFrmHeapHead::alloc(size_t size, s32 align) {
    if (size == 0) {
        size = 1;
    }
    size = RoundUp(size, 4);

    void *block = nullptr;
    if (align >= 0) {
        block = allocFromHead(size, align);
    } else {
        block = allocFromTail(size, -align);
    }

    return block;
}

/// @addr{0x801996A4}
/// @brief Gets the maximum size that can be allocated with the specified alignment
/// @param align The alignment requirement for the memory block
/// @return The maximum allocatable size
u32 MEMiFrmHeapHead::getAllocatableSize(s32 align) const {
    // Doesn't matter which direction it can be allocated from, take absolute value
    align = std::abs(align);
    void *block = RoundUp(m_headAllocator, align);
    return block > m_tailAllocator ? 0 : GetAddrNum(SubOffset(m_tailAllocator, GetAddrNum(block)));
}

/// @brief Finds the end of the used heap region
/// @return Pointer to the end of the used heap region
/// @details This is used by @ref Host::Context to discern the minimum `memcpy` size required to
/// preserve the entire state of the heap. Falls back to @ref getHeapEnd() if anything has been
/// allocated from the tail.
void *MEMiFrmHeapHead::calcUsedEnd() {
    return m_tailAllocator == getHeapEnd() ? m_headAllocator : getHeapEnd();
}

/// @addr{0x80199430}
/// @brief Creates a new frame heap handle at the specified start address with the given size and
/// flags
/// @param startAddress The address at which to construct the heap handle
/// @param size The size of the memory region for the heap
/// @param flag The flags to set for the heap
/// @return A pointer to the newly created frame heap handle, or `nullptr` if creation fails
MEMiFrmHeapHead *MEMiFrmHeapHead::Create(void *startAddress, size_t size, u16 flag) {
    void *endAddress = AddOffset(startAddress, size);

    startAddress = RoundUp(startAddress, 4);
    endAddress = RoundDown(endAddress, 4);

    uintptr_t startAddrNum = GetAddrNum(startAddress);
    uintptr_t endAddrNum = GetAddrNum(endAddress);

    if (startAddrNum > endAddrNum) {
        return nullptr;
    }

    if (endAddrNum - startAddrNum < sizeof(MEMiFrmHeapHead) + 4) {
        return nullptr;
    }

    return new (startAddress) MEMiFrmHeapHead(endAddress, flag);
}

/// @brief Constructor
/// @param end The end address of the memory region for the heap
/// @param opt The flags to set for the heap
MEMiFrmHeapHead::MEMiFrmHeapHead(void *end, u16 opt)
    : MEMiHeapHead(FRM_HEAP_SIGNATURE, AddOffset(this, sizeof(MEMiFrmHeapHead)), end, opt) {
    m_headAllocator = getHeapStart();
    m_tailAllocator = getHeapEnd();
}

/// @addr{0x801994B4}
/// @brief Default destructor
MEMiFrmHeapHead::~MEMiFrmHeapHead() = default;

/// @brief Allocates a memory block from the head of the heap
/// @param size The size of the memory block to allocate
/// @param alignment The alignment of the memory block
/// @return A pointer to the allocated block, or `nullptr` if there is not enough space
void *MEMiFrmHeapHead::allocFromHead(size_t size, s32 alignment) {
    void *start = RoundUp(m_headAllocator, alignment);
    void *end = AddOffset(start, size);

    if (end > m_tailAllocator) {
        return nullptr;
    }

    fillAllocMemory(start, size);
    m_headAllocator = end;
    return start;
}

/// @brief Allocates a memory block from the tail of the heap
/// @param size The size of the memory block to allocate
/// @param alignment The alignment of the memory block
/// @return A pointer to the allocated block, or `nullptr` if there is not enough space
void *MEMiFrmHeapHead::allocFromTail(size_t size, s32 alignment) {
    void *start = RoundDown(SubOffset(m_tailAllocator, size), alignment);

    if (start < m_headAllocator) {
        return nullptr;
    }

    fillAllocMemory(start, size);
    m_tailAllocator = start;
    return start;
}

} // namespace Kinoko::Abstract::Memory
