#pragma once

#include "egg/core/Heap.hh"

namespace Kinoko {

namespace Abstract::Memory {

class ExpHeap;
class MEMiExpHeapHead;

} // namespace Abstract::Memory

namespace EGG {

/// @brief Expanded heap implementation, allowing for allocation anywhere within the memory space it
/// manages.
/// @details Due to requesting chunks from a provided memory block, internal fragmentation is
/// non-existent, but external fragmentation is still possible. Allocating temporary blocks from the
/// tail, and scene-permanent blocks from the head, is recommended. The memory overhead per
/// allocation is `sizeof(Abstract::Memory::MEMiExpBlockHead)`.
class ExpHeap final : public Heap {
public:
    /// @brief Records the total size of memory blocks categorized by group ID
    /// @details Memory blocks have group IDs assigned to them (default 0). This class is a record
    /// of the total sum of those blocks' sizes, categorized by group ID. Used for memory profiling.
    /// Interfaces with the expanded heap via `ExpHeap::calcGroupSize`.
    class GroupSizeRecord {
    public:
        GroupSizeRecord();
        void reset();
        void addSize(u16 groupID, size_t size);

        /// @beginGetters

        [[nodiscard]] size_t size() const;
        [[nodiscard]] size_t getGroupSize(u16 groupID) const;

        /// @endGetters

    private:
        std::array<size_t, 256> m_entries; ///< The total size of memory blocks for each group ID
    };

    void destroy() override;

    [[nodiscard]] Kind getHeapKind() const override;
    [[nodiscard]] void *alloc(size_t size, s32 align) override;
    void free(void *block) override;
    [[nodiscard]] u32 getAllocatableSize(s32 align = 4) const override;

    void calcGroupSize(GroupSizeRecord *record);

    void setGroupID(u16 groupID);
    [[nodiscard]] u16 getGroupID() const;

    [[nodiscard]] Abstract::Memory::MEMiExpHeapHead *dynamicCastHandleToExp();
    [[nodiscard]] const Abstract::Memory::MEMiExpHeapHead *dynamicCastHandleToExp() const;

    static void addGroupSize(void *block, Abstract::Memory::MEMiHeapHead *heap, uintptr_t param);

    [[nodiscard]] static ExpHeap *Create(void *startAddress, size_t size, u16 opt);
    [[nodiscard]] static ExpHeap *Create(size_t size, Heap *heap, u16 opt);

private:
    ExpHeap(Abstract::Memory::MEMiHeapHead *handle);
    ~ExpHeap() override;
};

} // namespace EGG

} // namespace Kinoko
