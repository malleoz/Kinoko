#include "ExpHeap.hh"

#include <abstract/memory/ExpHeap.hh>

namespace Kinoko::EGG {

using namespace Abstract::Memory;

/// @addr{0x80226DD0}
/// @brief Constructor
/// @details Initializes @ref m_entries via @ref reset().
ExpHeap::GroupSizeRecord::GroupSizeRecord() {
    reset();
}

/// @addr{0x80226E00}
/// @brief Resets the size for all entries to zero
void ExpHeap::GroupSizeRecord::reset() {
    m_entries.fill(0);
}

/// @addr{0x80226E98}
/// @brief Adds the provided `size` to the entry for the provided `groupID`
/// @param groupID The ID of the entry to track the size for
/// @param size The size to add to the group's entry
void ExpHeap::GroupSizeRecord::addSize(u16 groupID, size_t size) {
    m_entries[groupID] += size;
}

/// @brief Gets the number of groups
/// @return The number of group entries (i.e. the size of @ref m_entries)
size_t ExpHeap::GroupSizeRecord::size() const {
    return m_entries.size();
}

/// @brief Gets the size of the memory block for the provided group ID
/// @param groupID The group to get the memory block size for
/// @return The size of the memory block for the provided group ID
size_t ExpHeap::GroupSizeRecord::getGroupSize(u16 groupID) const {
    return m_entries[groupID];
}

/// @addr{0x80226B94}
/// @brief Public method to destroy this heap
/// @details Calls this heap's destructor and frees the heap's memory block via @ref free().
void ExpHeap::destroy() {
    Heap *pParent = getParentHeap();
    this->~ExpHeap();
    if (pParent) {
        pParent->free(this);
    }
}

/// @addr{0x80226EFC}
/// @copybrief Heap::getHeapKind()
/// @return @ref Kind::Expanded
Heap::Kind ExpHeap::getHeapKind() const {
    return Kind::Expanded;
}

/// @addr{0x80226C04}
/// @brief Allocates a memory block of the provided size and alignment
/// @param size The size of the memory block to allocate
/// @param align The alignment requirement for the memory block
/// @return Pointer to the allocated memory block
/// @warning This function will `PANIC` is the heap is locked (indicated via the @ref eFlags::Lock
/// bit in @ref m_flags).
void *ExpHeap::alloc(size_t size, s32 align) {
    if (tstDisableAllocation()) {
        PANIC("HEAP ALLOC FAIL (%p, %s): Heap is locked", this, m_name);
    }

    return dynamicCastHandleToExp()->alloc(size, align);
}

/// @addr{0x80226C78}
void ExpHeap::free(void *block) {
    dynamicCastHandleToExp()->free(block);
}

/// @addr{0x80226C90}
u32 ExpHeap::getAllocatableSize(s32 align) const {
    return dynamicCastHandleToExp()->getAllocatableSize(align);
}

/// @addr{0x80226CFC}
void ExpHeap::calcGroupSize(GroupSizeRecord *record) {
    record->reset();
    dynamicCastHandleToExp()->visitAllocated(addGroupSize, GetAddrNum(record));
}

/// @addr{0x80226C98}
void ExpHeap::setGroupID(u16 groupID) {
    return dynamicCastHandleToExp()->setGroupID(groupID);
}

u16 ExpHeap::getGroupID() const {
    return dynamicCastHandleToExp()->getGroupID();
}

MEMiExpHeapHead *ExpHeap::dynamicCastHandleToExp() {
    return reinterpret_cast<MEMiExpHeapHead *>(m_handle);
}

const MEMiExpHeapHead *ExpHeap::dynamicCastHandleToExp() const {
    return reinterpret_cast<MEMiExpHeapHead *>(m_handle);
}

/// @addr{0x80226CA0}
void ExpHeap::addGroupSize(void *block, MEMiHeapHead * /* heap */, uintptr_t param) {
    MEMiExpBlockHead *blockHead =
            static_cast<MEMiExpBlockHead *>(SubOffset(block, sizeof(MEMiExpBlockHead)));
    u16 groupID = blockHead->m_attribute.fields.groupId;

    GroupSizeRecord *record = reinterpret_cast<GroupSizeRecord *>(param);
    record->addSize(groupID, blockHead->m_size);
}

/// @addr{0x80226A1C}
ExpHeap *ExpHeap::Create(void *startAddress, size_t size, u16 opt) {
    ExpHeap *heap = nullptr;

    void *endAddress = RoundDown(AddOffset(startAddress, size), 4);
    startAddress = RoundUp(startAddress, 4);

    size_t addrRange = GetAddrNum(endAddress) - GetAddrNum(startAddress);
    if (startAddress > endAddress || addrRange < sizeof(ExpHeap) + 4) {
        return nullptr;
    }

    void *handleStart = AddOffset(startAddress, sizeof(ExpHeap));
    MEMiExpHeapHead *handle =
            MEMiExpHeapHead::create(handleStart, addrRange - sizeof(ExpHeap), opt);
    if (handle) {
        heap = new (startAddress) ExpHeap(handle);
    }

    return heap;
}

/// @addr{0x80226AC8}
ExpHeap *ExpHeap::Create(size_t size, Heap *pHeap, u16 opt) {
    ExpHeap *heap = nullptr;

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
ExpHeap::ExpHeap(MEMiHeapHead *handle) : Heap(handle) {}

/// @addr{0x802269A8}
/// @brief Private virtual destructor
/// @details Destroys all children @ref Disposer objects via @ref dispose() and destroys @ref
/// m_handle.
ExpHeap::~ExpHeap() {
    dispose();
    dynamicCastHandleToExp()->destroy();
}

} // namespace Kinoko::EGG
