#pragma once

#include "abstract/memory/HeapCommon.hh"

namespace Kinoko::Abstract::Memory {

/// @brief Low-level implementation of a frame heap, which allocates sequentially from either end of
/// its memory space.
/// @details Allocations have no per-block overhead, but individual blocks cannot be freed.
class MEMiFrmHeapHead : public MEMiHeapHead {
public:
    void destroy();

    void *alloc(size_t size, s32 align);
    // Freeing by block is invalid
    // Freeing by flag is not implemented in Kinoko
    [[nodiscard]] u32 getAllocatableSize(s32 align) const;

    [[nodiscard]] void *calcUsedEnd();

    static MEMiFrmHeapHead *Create(void *startAddress, size_t size, u16 flag);

private:
    MEMiFrmHeapHead(void *end, u16 opt);
    ~MEMiFrmHeapHead();

    [[nodiscard]] void *allocFromHead(size_t size, s32 alignment);
    [[nodiscard]] void *allocFromTail(size_t size, s32 alignment);

    void *m_headAllocator; ///< The next address to allocate from the head
    void *m_tailAllocator; ///< The lowest address allocated from the tail

    static constexpr u32 FRM_HEAP_SIGNATURE = 0x46524d48; ///< Signature of a frame heap, "FRMH"
};

} // namespace Kinoko::Abstract::Memory
