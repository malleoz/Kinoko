#pragma once

#include "egg/core/Disposer.hh"

#include <abstract/memory/HeapCommon.hh>

#include <list>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace EGG {

class ExpHeap;

/// @brief A high-level representation of a memory heap for managing dynamic memory allocation.
/// Interface for allocating and freeing memory blocks.
class Heap : Disposer {
    /// @brief Grants access to static member variables so a @ref Host::Context can restore their
    /// values on context switch
    friend Host::Context;

public:
    /// @brief Distinguishes the type of heap
    /// @details This is used as a safety check when reinterpret casting.
    enum class Kind {
        None,     ///< Uninitialized or invalid
        Expanded, ///< Designates @ref ExpHeap
        Frame,    ///< Unused in Kinoko
        Unit,     ///< Unused in Kinoko
        Assert,   ///< Unused in Kinoko
    };

    Heap(Abstract::Memory::MEMiHeapHead *handle);
    ~Heap() override;

    /// @brief Destroys the heap and releases all associated resources
    virtual void destroy() = 0;

    /// @brief Returns the type of heap
    /// @return The @ref Kind of heap
    virtual Kind getHeapKind() const = 0;

    /// @brief Allocates a block of memory from the heap
    /// @param size The size of the memory block to allocate
    /// @param align The alignment requirement for the memory block
    /// @return A pointer to the allocated memory block
    virtual void *alloc(size_t size, s32 align) = 0;

    /// @brief Frees a previously allocated block of memory
    /// @param block A pointer to the memory block to free
    virtual void free(void *block) = 0;

    /// @brief Returns the maximum allocatable size for a given alignment
    /// @param align The alignment requirement
    /// @return The maximum size of memory that can be allocated with the specified alignment
    virtual u32 getAllocatableSize(s32 align = 4) const = 0;

    void dispose();

    void disableAllocation();
    void enableAllocation();

    [[nodiscard]] bool tstDisableAllocation() const;

    void appendDisposer(Disposer *disposer);
    void removeDisposer(Disposer *disposer);

    Heap *becomeCurrentHeap();

    [[nodiscard]] void *getStartAddress();
    [[nodiscard]] void *getEndAddress();
    [[nodiscard]] const char *getName() const;
    [[nodiscard]] Heap *getParentHeap() const;

    void setName(const char *name);
    void setParentHeap(Heap *heap);

    [[nodiscard]] static void *Alloc(size_t size, int align, Heap *pHeap);
    static void Free(void *block, Heap *pHeap);

    [[nodiscard]] static Heap *FindHeap(Abstract::Memory::MEMiHeapHead *handle);
    [[nodiscard]] static Heap *FindContainHeap(const void *block);

    [[nodiscard]] static ExpHeap *DynamicCastToExp(Heap *heap);
    [[nodiscard]] static Heap *GetCurrentHeap();

    /// @brief Gets the offset of the @ref m_link member from the start of the @ref Heap object
    /// @return The offset of the @ref m_link member
    /// @details The C++ standard only guarantees `offsetof` for standard-layout types. Since @ref
    /// Heap has virtual functions, `offset` of is not universally supported. Instead of hardcoding
    /// an offset, we derive it ourselves.
    [[nodiscard]] static constexpr uintptr_t getOffset() {
        return reinterpret_cast<uintptr_t>(&reinterpret_cast<Heap *>(NULL)->m_link);
    }

protected:
    /// @brief Flags used to control heap behavior
    enum class eFlags {
        Lock = 0, ///< The heap is locked and cannot allocate memory
    };

    /// @brief A bitfield of @ref eFlags used to control heap behavior
    typedef TBitFlag<u8, eFlags> Flags;

    Abstract::Memory::MEMiHeapHead *const m_handle; ///< Handler for managing / accessing this heap
    Heap *m_parentHeap;                             ///< The parent heap, or `nullptr` if no parent
    Flags m_flags;                                  ///< Flags used to control heap behavior
    Abstract::Memory::MEMLink m_link;     ///< This heap's link in the global heap linked list
    Abstract::Memory::MEMList m_children; ///< List of @ref Disposer objects managed by this heap
    const char *m_name;                   ///< The name of this heap

    /// @addr{0x80384320}
    /// @brief The global linked list of all heaps
    static Abstract::Memory::MEMList s_heapList;

    /// @addr{0x80386EA0}
    /// @brief The global default heap used when an allocator does not specify a heap
    static Heap *s_currentHeap;

    /// @addr{0x80386EA8}
    /// @brief When set, acts as a global allocation guard, preventing allocation to any other heap
    static Heap *s_allocatableHeap;
};

[[nodiscard]] void *egg_alloc(size_t size, s32 align = 4, Heap *pHeap = nullptr);
void egg_free(void *block, Heap *pHeap = nullptr);

/// @brief Allocates and constructs an object of type `T` using the EGG heap system
/// @tparam T The type of object to allocate and construct
/// @tparam ...Args The types of arguments to pass to the constructor of `T`
/// @param ...args The arguments to pass to the constructor of `T`
/// @return A pointer to the newly allocated and constructed object of type `T`
/// @details Uses `placement new` to construct the object in the memory allocated from the EGG heap
/// system.
template <typename T, typename... Args>
[[nodiscard]] T *egg_new(Args &&...args) {
    return ::new (egg_alloc(sizeof(T), static_cast<s32>(alignof(T))))
            T(std::forward<Args>(args)...);
}

/// @brief Destroys an object which was allocated using the EGG heap system
/// @tparam T The type of object to destroy
/// @param ptr A pointer to the object to destroy
template <typename T>
void egg_delete(const T *ptr) {
    if (ptr) {
        ptr->~T();
        egg_free(const_cast<T *>(ptr));
    }
}

/// @brief Allocates and default constructs an array of objects of type `T` using the EGG heap
/// system
/// @tparam T The type of objects to construct
/// @param count The number of objects to allocate and construct
/// @return A pointer to the newly allocated and constructed array of objects of type `T`
/// @details Uses `placement new` to construct the array of objects in the memory allocated from the
/// EGG heap system.
template <typename T>
[[nodiscard]] T *egg_new_array(size_t count) {
    T *p = static_cast<T *>(egg_alloc(sizeof(T) * count, static_cast<s32>(alignof(T))));
    for (size_t i = 0; i < count; ++i) {
        ::new (p + i) T();
    }
    return p;
}

/// @brief Destroys an array of objects which was allocated using the EGG heap system
/// @tparam T The type of objects in the array to destroy
/// @param ptr A pointer to the array of objects to destroy
template <typename T>
void egg_delete_array(T *ptr, size_t count) {
    if (ptr) {
        for (size_t i = count; i-- > 0;) {
            ptr[i].~T();
        }
        egg_free(ptr);
    }
}

} // namespace EGG

} // namespace Kinoko

/// @brief Grants Kinoko::EGG::egg_new and Kinoko::EGG::egg_delete access to a class' private
/// constructors/destructor, without making them public.
#define EGG_NEW_DELETE_FRIEND \
    template <typename T, typename... Args> \
    friend T *Kinoko::EGG::egg_new(Args &&...args); \
    template <typename T> \
    friend void Kinoko::EGG::egg_delete(const T *ptr);
