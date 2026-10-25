#pragma once

#include "abstract/memory/List.hh"

#include <egg/core/BitFlag.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Abstract::Memory {

/// @brief A handler for accessing and managing a @ref Heap
class MEMiHeapHead {
    /// @brief Grants access to @ref s_rootList so a @ref Host::Context can reload the list on
    /// context switch
    friend Host::Context;

public:
    /// @brief Distinguishes the type of byte pattern that should be written when @ref
    /// eOptFlag::DebugFillAlloc is enabled
    enum class FillType {
        NoUse = 0, ///< Unallocated/recycled memory block
        Alloc = 1, ///< Allocated memory block
        Free = 2,  ///< Recycled memory block
    };

    /// @brief Various flags to control behavior for heap management
    enum class eOptFlag {
        ZeroFillAlloc = 0,  ///< Zero-initializes all bytes in the heap upon allocation
        DebugFillAlloc = 1, ///< Initializes all bytes in the heap based on the heap's @ref FillType
    };

    /// @brief A bitfield of @ref eOptFlag that represents the options for the heap
    typedef EGG::TBitFlag<u8, eOptFlag> OptFlag;

    [[nodiscard]] MEMList &getChildList();
    [[nodiscard]] void *getHeapStart();
    [[nodiscard]] void *getHeapEnd();

    [[nodiscard]] static MEMList &GetRootList();
    [[nodiscard]] static MEMiHeapHead *FindContainHeap(const void *block);

    /// @brief Gets the offset of the link member within the @ref Archive class
    /// @return The offset of the link member within the @ref Archive class
    [[nodiscard]] static constexpr u16 getLinkOffset() {
        return offsetof(MEMiHeapHead, m_link);
    }

    /// @brief Maps the provided @ref FillType to a unique 4-byte fill value
    /// @param type The type of byte pattern to fill with
    /// @return The 4-byte fill value corresponding to the provided @ref FillType
    /// @details This is used for easier debugging in Kinoko. If an invalid memory read-write
    /// occurs, the fill values help identify the type of memory involved.
    [[nodiscard]] static constexpr u32 GetFillVal(FillType type) {
        size_t t = static_cast<std::underlying_type_t<FillType>>(type);
        ASSERT(t < 3);
        return s_fillVals[t];
    }

protected:
    MEMiHeapHead(u32 signature, void *heapStart, void *heapEnd, const OptFlag &opt);
    ~MEMiHeapHead();

    void fillNoUseMemory(void *address, u32 size);
    void fillAllocMemory(void *address, u32 size);
    void fillFreeMemory(void *address, u32 size);

private:
    [[nodiscard]] static MEMiHeapHead *FindContainHeap(MEMList *list, const void *block);
    [[nodiscard]] MEMList &findListContainHeap() const;

    const u32 m_signature;   ///< Unique signature indicating the type of heap
    const OptFlag m_optFlag; ///< Options for the heap
    MEMLink m_link;          ///< This heap's link in the linked list of heaps
    MEMList m_childList;     ///< List of all heap children
    void *const m_heapStart; ///< Pointer to the start of this heap
    void *const m_heapEnd;   ///< Pointer to the end of this heap

    /// @brief Global list of all root heaps
    /// @details For Kinoko, this will only ever contain the "EGGRoot" heap.
    static MEMList s_rootList;

    /// @brief Array of values to write to heap blocks when @ref eOptFlag::DebugFillAlloc is set,
    /// indexed by @ref FillType
    static constexpr std::array<u32, 3> s_fillVals = {{
            0xC3C3C3C3,
            0xF3F3F3F3,
            0xD3D3D3D3,
    }};
};

/// @brief The size of memory blocks that are allocated for game heap space.
static constexpr size_t MEMORY_SPACE_SIZE = 0x1000000;

#ifdef BUILD_DEBUG
static constexpr auto DEFAULT_OPT = MEMiHeapHead::OptFlag(MEMiHeapHead::eOptFlag::DebugFillAlloc);
#else
/// @brief Default options for new heaps
/// @details Unlike the base game, heaps are not zero-initialized, for performance. Members which
/// the base game relies on being zero-initialized are explicitly initialized instead.
static constexpr auto DEFAULT_OPT = MEMiHeapHead::OptFlag();
#endif

} // namespace Abstract::Memory

} // namespace Kinoko
