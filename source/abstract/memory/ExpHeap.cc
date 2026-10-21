#include "ExpHeap.hh"

/// @brief Houses various memory management utilities including heaps and their allocators.
namespace Kinoko::Abstract::Memory {

/// @brief Constructor
/// @param start Pointer to the start of the region
/// @param end Pointer to the end of the region
/// @details Sets @ref start to `start_` and `end` to `end_`
Region::Region(void *start_, void *end_) : start(start_), end(end_) {}

/// @brief Calculates the number of bytes in the memory region
/// @return The number of bytes in the memory region
uintptr_t Region::getRange() const {
    return GetAddrNum(end) - GetAddrNum(start);
}

/// @brief Inserts a memory block after the specified `prev` block in the list
/// @param block The memory block to insert
/// @param prev The memory block to insert this new block after
/// @return The inserted memory block
/// @details If `prev` is `nullptr`, then the block becomes the @ref m_head of the list. If `block`
/// does not have a next block, it becomes the @ref m_tail of the list.
MEMiExpBlockHead *MEMiExpBlockList::insert(MEMiExpBlockHead *block, MEMiExpBlockHead *prev) {
    MEMiExpBlockHead *next;

    block->m_link.m_prev = prev;
    if (prev) {
        next = prev->m_link.m_next;
        prev->m_link.m_next = block;
    } else {
        next = m_head;
        m_head = block;
    }

    block->m_link.m_next = next;
    if (next) {
        next->m_link.m_prev = block;
    } else {
        m_tail = block;
    }

    return block;
}

/// @brief Appends a memory block to the end of the list
/// @param block The memory block to append
/// @return The appended memory block
/// @details Calls @ref insert() with `prev` set to the current @ref m_tail of the list.
MEMiExpBlockHead *MEMiExpBlockList::append(MEMiExpBlockHead *block) {
    return insert(block, m_tail);
}

/// @brief Removes a memory block from its containing block list
/// @param block The memory block to remove
/// @return The previous memory block in the list, or `nullptr` if `block` was the head of the list
/// @details If `block` is the head of the list, the next block becomes the new head. If `block` is
/// the tail of the list, the previous block becomes the new tail.
MEMiExpBlockHead *MEMiExpBlockList::remove(MEMiExpBlockHead *block) {
    MEMiExpBlockHead *prev = block->m_link.m_prev;
    MEMiExpBlockHead *next = block->m_link.m_next;

    if (prev) {
        prev->m_link.m_next = next;
    } else {
        m_head = next;
    }

    if (next) {
        next->m_link.m_prev = prev;
    } else {
        m_tail = prev;
    }

    return prev;
}

/// @brief Private constructor
/// @param region The region of the memory block
/// @param signature The signature of the memory block ("FR" for free, "UD" for used)
/// @details Sets @ref m_signature to the provided `signature`. Sets @ref m_size to the size of the
/// `region` minus `sizeof(MEMiExpBlockHead)`. Initializes the block's attributes to zero and the
/// block's link pointers to `nullptr`.
MEMiExpBlockHead::MEMiExpBlockHead(const Region &region, u16 signature)
    : m_signature(signature),
      m_size(region.getRange() - sizeof(MEMiExpBlockHead)) {
    m_attribute.val = 0;

    m_link.m_prev = nullptr;
    m_link.m_next = nullptr;
}

/// @brief Default destructor
MEMiExpBlockHead::~MEMiExpBlockHead() = default;

/// @brief Creates a free block of memory
/// @param region The region of the block to designate as free
/// @return Pointer to the created free memory block
/// @details Constructs a `MEMiExpBlockHead` with the signature for a free block ("FR") at the start
/// of the provided `region`. Free blocks are designated as available blocks that can be allocated
/// for future use.
MEMiExpBlockHead *MEMiExpBlockHead::createFree(const Region &region) {
    constexpr u16 FREE_BLOCK_SIGNATURE = 0x4652; // FR
    return new (region.start) MEMiExpBlockHead(region, FREE_BLOCK_SIGNATURE);
}

/// @brief Creates a used block of memory
/// @param region The region of the block to designate as used
/// @return Pointer to the created used memory block
/// @details Constructs a `MEMiExpBlockHead` with the signature for a used block ("UD") at the start
/// of the provided `region`. Used blocks represent heaps that are currently allocated and in use.
MEMiExpBlockHead *MEMiExpBlockHead::createUsed(const Region &region) {
    constexpr u16 USED_BLOCK_SIGNATURE = 0x5544; // UD
    return new (region.start) MEMiExpBlockHead(region, USED_BLOCK_SIGNATURE);
}

/// @brief Gets the @ref Region of this memory block
/// @return The @ref Region representing the memory block's start and end addresses
Region MEMiExpBlockHead::getRegion() const {
    return Region(SubOffset(this, m_attribute.fields.alignment), getMemoryEnd());
}

/// @brief Gets the start of this memory block's data (after the @ref MEMiExpBlockHead)
/// @return Pointer to the start of the memory block's usable data
void *MEMiExpBlockHead::getMemoryStart() const {
    return AddOffset(this, sizeof(MEMiExpBlockHead));
}

/// @brief Gets the end of this memory block's data
/// @return Pointer to the end of the memory block's usable data
void *MEMiExpBlockHead::getMemoryEnd() const {
    return AddOffset(getMemoryStart(), m_size);
}

/// @brief Destroys this memory heap
/// @details Calls the @ref MEMiExpHeapHead destructor on this instance to clean up the heap's
/// handler information.
void MEMiExpHeapHead::destroy() {
    this->~MEMiExpHeapHead();
}

/// @addr{0x80198D88}
/// @brief Allocates a memory block of the specified size and alignment
/// @param size The size of the memory block to allocate
/// @param align The alignment requirement for the memory block. Positive values indicate alignment
/// from the head, negative values indicate alignment from the tail.
/// @return Pointer to the allocated memory block, or `nullptr` if allocation fails
void *MEMiExpHeapHead::alloc(size_t size, s32 align) {
    if (size == 0) {
        size = 1;
    }
    size = RoundUp(size, 4);

    return align >= 0 ? allocFromHead(size, align) : allocFromTail(size, -align);
}

/// @addr{0x80199038}
/// @brief Frees a previously allocated memory block
/// @param block Pointer to the memory block to free
/// @details Does nothing if `block` is `nullptr`. Retrieves the @ref MEMiExpBlockHead corresponding
/// to the memory block and recycles its region via @ref recycleRegion().
void MEMiExpHeapHead::free(void *block) {
    if (!block) {
        return;
    }

    MEMiExpBlockHead *head =
            reinterpret_cast<MEMiExpBlockHead *>(SubOffset(block, sizeof(MEMiExpBlockHead)));

    Region region = head->getRegion();
    m_usedBlocks.remove(head);
    recycleRegion(region);
}

/// @addr{0x80199180}
/// @brief Returns the maximum allocatable size for a given alignment requirement
/// @param align The alignment requirement
/// @return The maximum size of a memory block that can be allocated with the specified alignment
/// @details Traverses all @ref m_freeBlocks and calculates the maximum allocatable size considering
/// the alignment requirement.
u32 MEMiExpHeapHead::getAllocatableSize(s32 align) const {
    // Doesn't matter which direction it can be allocated from, take absolute value
    align = std::abs(align);

    u32 maxSize = 0;
    u32 x = std::numeric_limits<u32>::max();

    for (MEMiExpBlockHead *block = m_freeBlocks.m_head; block; block = block->m_link.m_next) {
        void *memptr = block->getMemoryStart();
        void *start = RoundUp(memptr, align);
        void *end = block->getMemoryEnd();

        if (GetAddrNum(start) < GetAddrNum(end)) {
            u32 size = GetAddrNum(end) - GetAddrNum(start);
            u32 offset = GetAddrNum(start) - GetAddrNum(memptr);

            if (maxSize < size || (maxSize == size && x > offset)) {
                maxSize = size;
                x = offset;
            }
        }
    }

    return maxSize;
}

/// @addr{0x801992A8}
/// @brief Visits all allocated memory blocks and evaluates the provided `visitor` function on each
/// block
/// @param visitor The function to evaluate on each allocated memory block
/// @param param An additional parameter to pass to the visitor function
void MEMiExpHeapHead::visitAllocated(Visitor visitor, uintptr_t param) {
    for (MEMiExpBlockHead *block = m_usedBlocks.m_head; block;) {
        MEMiExpBlockHead *next = block->m_link.m_next;
        visitor(block->getMemoryStart(), this, param);
        block = next;
    }
}

/// @addr{0x8019934C}
/// @brief Retrieves the @ref GroupID of the heap
/// @return The group ID of the heap, @ref m_groupID
u16 MEMiExpHeapHead::getGroupID() const {
    return m_groupId;
}

/// @addr{0x80199258}
/// @brief Sets the @ref GroupID of the heap
/// @param groupID The group ID to set for the heap
void MEMiExpHeapHead::setGroupID(u16 groupID) {
    m_groupId = groupID;
}

/// @addr{0x80198CA8}
/// @brief Creates a new expandable heap handle at the specified start address with the given size
/// and flags
/// @param startAddress The address at which to construct the heap handle
/// @param size The size of the memory region for the heap
/// @param flag The flags to set for the heap
/// @return A pointer to the newly created expandable heap handle, or `nullptr` if creation fails
/// @details Fails if the specified memory region is too small to accommodate the heap handle and
/// its initial block.
MEMiExpHeapHead *MEMiExpHeapHead::Create(void *startAddress, size_t size, u16 flag) {
    void *endAddress = AddOffset(startAddress, size);

    startAddress = RoundUp(startAddress, 4);
    endAddress = RoundDown(endAddress, 4);

    uintptr_t startAddrNum = GetAddrNum(startAddress);
    uintptr_t endAddrNum = GetAddrNum(endAddress);

    if (startAddrNum > endAddrNum) {
        return nullptr;
    }

    if (endAddrNum - startAddrNum < sizeof(MEMiExpHeapHead) + sizeof(MEMiExpBlockHead) + 4) {
        return nullptr;
    }

    return new (startAddress) MEMiExpHeapHead(endAddress, flag);
}

/// @brief Constructor
/// @param end The end address of the memory region for the heap
/// @param opt The flags to set for the heap
/// @details Initializes @ref m_groupId to zero and clears all flags in @ref m_attribute. Marks the
/// memory block of the new heap as free via @ref MEMiExpBlockHead::createFree(). Sets this block as
/// both the head and tail of @ref m_freeBlocks and initializes the head and tail of @ref
/// m_usedBlocks to `nullptr`.
MEMiExpHeapHead::MEMiExpHeapHead(void *end, u16 opt)
    : MEMiHeapHead(EXP_HEAP_SIGNATURE, AddOffset(this, sizeof(MEMiExpHeapHead)), end, opt) {
    m_groupId = 0;
#ifdef BUILD_DEBUG
    m_tag = 0;
#endif // BUILD_DEBUG
    m_attribute.makeAllZero();

    Region region = Region(getHeapStart(), getHeapEnd());
    MEMiExpBlockHead *block = MEMiExpBlockHead::createFree(region);

    m_freeBlocks.m_head = block;
    m_freeBlocks.m_tail = block;
    m_usedBlocks.m_head = nullptr;
    m_usedBlocks.m_tail = nullptr;
}

/// @addr{0x80198D58}
/// @brief Default destructor
MEMiExpHeapHead::~MEMiExpHeapHead() = default;

/// @addr{0x8019899C}
/// @brief Allocates a memory block from the head of the heap
/// @param size The size of the memory block to allocate
/// @param alignment The alignment requirement for the memory block
/// @return A pointer to the allocated memory block, or `nullptr` if allocation fails
/// @details Iterates all free memory blocks from the head of the free list to find a suitable block
/// that satisfies the size and alignment requirements. If @ref eAttribute::BestFitAlloc is set in
/// @ref m_attribute, it selects the smallest block that fits the request.
void *MEMiExpHeapHead::allocFromHead(size_t size, s32 alignment) {
    MEMiExpBlockHead *found = nullptr;
    u32 blockSize = -1;
    void *bestAddress = nullptr;

    for (MEMiExpBlockHead *block = m_freeBlocks.m_head; block; block = block->m_link.m_next) {
        void *const memptr = block->getMemoryStart();
        void *const address = RoundUp(memptr, alignment);
        size_t offset = GetAddrNum(address) - GetAddrNum(memptr);

        if (block->m_size < size + offset || blockSize <= block->m_size) {
            continue;
        }

        found = block;
        blockSize = block->m_size;
        bestAddress = address;

        // This is a valid block to allocate in, but is it the best one?
        if (m_attribute.offBit(eAttribute::BestFitAlloc) || blockSize == size) {
            break;
        }
    }

    return found ? allocUsedBlockFromFreeBlock(found, bestAddress, size, 0) : nullptr;
}

/// @addr{0x80198A78}
/// @brief Allocates a memory block from the tail of the heap
/// @param size The size of the memory block to allocate
/// @param alignment The alignment requirement for the memory block
/// @return A pointer to the allocated memory block, or `nullptr` if allocation fails
/// @details Iterates all free memory blocks from the tail of the free list to find a suitable block
/// that satisfies the size and alignment requirements. If @ref eAttribute::BestFitAlloc is set in
/// @ref m_attribute, it selects the smallest block that fits the request.
void *MEMiExpHeapHead::allocFromTail(size_t size, s32 alignment) {
    MEMiExpBlockHead *found = nullptr;
    u32 blockSize = -1;
    void *bestAddress = nullptr;

    for (MEMiExpBlockHead *block = m_freeBlocks.m_tail; block; block = block->m_link.m_prev) {
        void *const start = block->getMemoryStart();
        void *const endAddr = AddOffset(start, block->m_size);
        void *const end = RoundDown(SubOffset(endAddr, size), alignment);

        if (GetAddrNum(end) < GetAddrNum(start) || blockSize <= block->m_size) {
            continue;
        }

        found = block;
        blockSize = block->m_size;
        bestAddress = end;

        // This is a valid block to allocate in, but is it the best one?
        if (m_attribute.offBit(eAttribute::BestFitAlloc) || blockSize == size) {
            break;
        }
    }

    return found ? allocUsedBlockFromFreeBlock(found, bestAddress, size, 1) : nullptr;
}

/// @addr{0x80198798}
/// @brief Creates a used memory block from a free memory block
/// @param block The free memory block to allocate from
/// @param address The starting address of the memory block to allocate
/// @param size The size of the memory block to allocate
/// @param direction The allocation direction, 0 for head, 1 for tail
/// @return A pointer to the allocated memory block, or `nullptr` if allocation fails
/// @details This function takes a free memory block and splits it into a used block and potentially
/// two smaller free blocks on either side, depending on the provided `address` and size. We start
/// by removing the free block from @ref m_freeBlocks. It then checks if there is enough space to
/// create new free blocks on the left and right sides of the allocated block. If there is enough
/// space, it inserts the new free blocks back into the free list. Finally, it creates a new used
/// block for the requested allocation and appends it to @ref m_usedBlocks.
void *MEMiExpHeapHead::allocUsedBlockFromFreeBlock(MEMiExpBlockHead *block, void *address, u32 size,
        s32 direction) {
    // The left region represents the free block created to the left of the new memory block
    // The right region represents the free block created to the right of the new memory block
    // address -> address + size exists entirely between leftRegion.end and rightRegion.start
    Region leftRegion = block->getRegion();
    Region rightRegion = Region(AddOffset(address, size), leftRegion.end);
    leftRegion.end = SubOffset(address, sizeof(MEMiExpBlockHead));

    MEMiExpBlockHead *prev = m_freeBlocks.remove(block);

    if (leftRegion.getRange() < sizeof(MEMiExpBlockHead) + 4) {
        // Not enough room to insert a free memory block
        leftRegion.end = leftRegion.start;
    } else {
        prev = m_freeBlocks.insert(MEMiExpBlockHead::createFree(leftRegion), prev);
    }

    if (rightRegion.getRange() < sizeof(MEMiExpBlockHead) + 4) {
        // Not enough room to insert a free memory block
        rightRegion.end = rightRegion.start;
    } else {
        m_freeBlocks.insert(MEMiExpBlockHead::createFree(rightRegion), prev);
    }

    fillAllocMemory(leftRegion.end, GetAddrNum(rightRegion.start) - GetAddrNum(leftRegion.end));

    // Now that the free blocks are cleared away, create a new used block
    Region region = Region(SubOffset(address, sizeof(MEMiExpBlockHead)), rightRegion.start);
    MEMiExpBlockHead *head = MEMiExpBlockHead::createUsed(region);

    head->m_attribute.fields.direction = direction;
    head->m_attribute.fields.alignment = GetAddrNum(head) - GetAddrNum(leftRegion.end);
    head->m_attribute.fields.groupId = m_groupId;
#ifdef BUILD_DEBUG
    head->m_tag = ++m_tag;
#endif // BUILD_DEBUG

    m_usedBlocks.append(head);

    return address;
}

/// @addr{0x80198B40}
/// @brief Recycles a used region of memory back into the free list of the expanded heap
/// @param initialRegion The region of memory to recycle
/// @return `true` if the region was successfully recycled, `false` otherwise
/// @details This function attempts to merge the given used memory region back into the free list.
/// It searches for adjacent free blocks to potentially merge with, and if the resulting region is
/// large enough to hold a free block header, it inserts it back into the free list. If the region
/// is too small, the function returns `false`.
bool MEMiExpHeapHead::recycleRegion(const Region &initialRegion) {
    MEMiExpBlockHead *block = nullptr;
    Region region = initialRegion;

    for (MEMiExpBlockHead *search = m_freeBlocks.m_head; search; search = search->m_link.m_next) {
        if (search < initialRegion.start) {
            block = search;
            continue;
        }

        if (search == initialRegion.end) {
            region.end = search->getMemoryEnd();
            m_freeBlocks.remove(search);
            fillNoUseMemory(search, sizeof(MEMiExpBlockHead));
        }

        break;
    }

    if (block && block->getMemoryEnd() == initialRegion.start) {
        region.start = block;
        block = m_freeBlocks.remove(block);
    }

    if (region.getRange() < sizeof(MEMiExpBlockHead)) {
        return false;
    }

    fillFreeMemory(initialRegion.start, initialRegion.getRange());
    m_freeBlocks.insert(MEMiExpBlockHead::createFree(region), block);
    return true;
}

} // namespace Kinoko::Abstract::Memory
