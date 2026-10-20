#pragma once

#include "abstract/memory/HeapCommon.hh"

#include <functional>

namespace Kinoko::Abstract::Memory {

/// @brief Represents a region of memory.
struct Region {
    void *start; ///< Pointer to the start of the region
    void *end;   ///< Pointer to the end of the region

    Region(void *start_, void *end_);
    [[nodiscard]] uintptr_t getRange() const;
};

struct MEMiExpBlockHead;

/// @brief Link of a non-intrusive doubly-linked list of memory blocks.
struct MEMiExpBlockLink {
    MEMiExpBlockHead *m_prev; ///< Pointer to the previous block in the list
    MEMiExpBlockHead *m_next; ///< Pointer to the next block in the list
};

/// @brief Non-intrusive doubly-linked list of memory blocks
struct MEMiExpBlockList {
    MEMiExpBlockHead *m_head; ///< Pointer to the first block in the list
    MEMiExpBlockHead *m_tail; ///< Pointer to the last block in the list

    MEMiExpBlockHead *insert(MEMiExpBlockHead *block, MEMiExpBlockHead *prev);
    MEMiExpBlockHead *append(MEMiExpBlockHead *block);
    MEMiExpBlockHead *remove(MEMiExpBlockHead *block);
};

/// @brief Head of the memory block. Contains information about the block and a link in the
/// corresponding used/free linked lists.
struct MEMiExpBlockHead {
public:
    const u16 m_signature; ///< Signature of the memory block, "FR" for free, "UD" for used
    union {
        u16 val; ///< Raw 2-byte value representing the block attributes
        struct {
            u16 direction : 1; ///< Allocation direction, 0 for head, 1 for tail
            u16 alignment : 7; ///< Alignment of the memory block
            u16 groupId : 8;   ///< The @ref GroupID of the memory block
        } fields;              ///< Structured view of the block attributes
    } m_attribute;             ///< Union of block attributes
    const u32 m_size;          ///< Size of the memory block (minus `sizeof(MEMiExpBlockHead)`)
#ifdef BUILD_DEBUG
    u32 m_tag;
#endif
    MEMiExpBlockLink m_link; ///< This block's link in the used/free linked list

    [[nodiscard]] Region getRegion() const;
    [[nodiscard]] void *getMemoryStart() const;
    [[nodiscard]] void *getMemoryEnd() const;

    [[nodiscard]] static MEMiExpBlockHead *createFree(const Region &region);
    [[nodiscard]] static MEMiExpBlockHead *createUsed(const Region &region);

private:
    MEMiExpBlockHead(const Region &region, u16 signature);
    ~MEMiExpBlockHead();
};

/// @brief Low-level implementation of a memory heap for managing dynamic memory allocation.
/// Allocation may occur anywhere within the memory space the heap is created from.
/// @details Due to requesting chunks from a provided memory block, internal fragmentation is
/// non-existent, but external fragmentation is still possible. Allocating temporary blocks from the
/// tail, and scene-permanent blocks from the head, is recommended. The memory overhead per
/// allocation is `sizeof(MEMiExpBlockHead)`.
class MEMiExpHeapHead : public MEMiHeapHead {
public:
    /// @brief Visitor function type for iterating over allocated memory blocks
    /// @param void* Pointer to the memory block
    /// @param MEMiHeapHead* Pointer to the heap containing the block
    /// @param uintptr_t User-defined parameter passed to the visitor function
    typedef std::function<void(void *startAddress, MEMiHeapHead *heap, uintptr_t param)> Visitor;

    void destroy();
    void *alloc(size_t size, s32 align);
    void free(void *block);
    [[nodiscard]] u32 getAllocatableSize(s32 align) const;
    void visitAllocated(Visitor visitor, uintptr_t param);

    /// @beginGetters

    [[nodiscard]] u16 getGroupID() const;

    /// @endGetters

    /// @beginSetters

    void setGroupID(u16 groupID);

    /// @endSetters

    static MEMiExpHeapHead *Create(void *startAddress, size_t size, u16 flag);

private:
    /// @brief Attributes controlling the behavior of the expanded heap
    enum class eAttribute {
        BestFitAlloc = 0, ///< Reduces internal fragmentation by selecting the best-fit free block
    };

    /// @brief Bitfield of @ref eAttribute flags representing the attributes of the expanded heap
    typedef EGG::TBitFlag<u8, eAttribute> Attribute;

    MEMiExpHeapHead(void *end, u16 opt);
    ~MEMiExpHeapHead();

    [[nodiscard]] void *allocFromHead(size_t size, s32 alignment);
    [[nodiscard]] void *allocFromTail(size_t size, s32 alignment);
    [[nodiscard]] void *allocUsedBlockFromFreeBlock(MEMiExpBlockHead *block, void *address,
            u32 size, s32 direction);
    bool recycleRegion(const Region &initialRegion);

    MEMiExpBlockList m_freeBlocks; ///< Linked list of free memory blocks
    MEMiExpBlockList m_usedBlocks; ///< Linked list of used memory blocks
    u16 m_groupId;                 ///< The @ref GroupID of this heap
#ifdef BUILD_DEBUG
    u32 m_tag;
#endif
    Attribute m_attribute; ///< Attributes controlling the behavior of the expanded heap

    static constexpr u32 EXP_HEAP_SIGNATURE = 0x45585048; ///< Signature of an expanded heap, "EXPH"
};

} // namespace Kinoko::Abstract::Memory
