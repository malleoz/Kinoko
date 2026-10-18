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
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    enum class Kind {
        None,
        Expanded,
        Frame,
        Unit,
        Assert,
    };

    Heap(Abstract::Memory::MEMiHeapHead *handle);
    ~Heap() override;

    virtual void destroy() = 0;
    virtual Kind getHeapKind() const = 0;
    virtual void *alloc(size_t size, s32 align) = 0;
    virtual void free(void *block) = 0;
    virtual u32 getAllocatableSize(s32 align = 4) const = 0;

    void dispose();

    void disableAllocation();
    void enableAllocation();

    [[nodiscard]] bool tstDisableAllocation() const;

    void appendDisposer(Disposer *disposer);
    void removeDisposer(Disposer *disposer);

    Heap *becomeAllocatableHeap();
    Heap *becomeCurrentHeap();

    void registerHeapBuffer(void *buffer);
    [[nodiscard]] void *getStartAddress();
    [[nodiscard]] void *getEndAddress();
    [[nodiscard]] const char *getName() const;
    [[nodiscard]] Heap *getParentHeap() const;

    void setName(const char *name);
    void setParentHeap(Heap *heap);

    [[nodiscard]] static void *alloc(size_t size, int align, Heap *pHeap);
    static void free(void *block, Heap *pHeap);

    [[nodiscard]] static Heap *findHeap(Abstract::Memory::MEMiHeapHead *handle);
    [[nodiscard]] static Heap *findContainHeap(const void *block);

    [[nodiscard]] static ExpHeap *dynamicCastToExp(Heap *heap);
    [[nodiscard]] static Heap *getCurrentHeap();

    [[nodiscard]] static constexpr uintptr_t getOffset() {
        // offsetof doesn't work, so instead of hardcoding an offset, we derive it ourselves
        return reinterpret_cast<uintptr_t>(&reinterpret_cast<Heap *>(NULL)->m_link);
    }

protected:
    enum class eFlags {
        Lock = 0,
    };
    typedef TBitFlag<u16, eFlags> Flags;

    Abstract::Memory::MEMiHeapHead *m_handle;
    void *m_block;
    Heap *m_parentHeap;
    Flags m_flags;
    Abstract::Memory::MEMLink m_link;
    Abstract::Memory::MEMList m_children;
    const char *m_name;

    static Abstract::Memory::MEMList s_heapList;

    static Heap *s_currentHeap;
    static Heap *s_allocatableHeap;
};

[[nodiscard]] void *egg_alloc(size_t size, s32 align = 4, Heap *pHeap = nullptr);
void egg_free(void *block, Heap *pHeap = nullptr);

template <typename T, typename... Args>
[[nodiscard]] T *egg_new(Args &&...args) {
    return ::new (egg_alloc(sizeof(T), static_cast<s32>(alignof(T))))
            T(std::forward<Args>(args)...);
}

template <typename T>
void egg_delete(const T *ptr) {
    if (ptr) {
        ptr->~T();
        egg_free(const_cast<T *>(ptr));
    }
}

template <typename T>
[[nodiscard]] T *egg_new_array(size_t count) {
    T *p = static_cast<T *>(egg_alloc(sizeof(T) * count, static_cast<s32>(alignof(T))));
    for (size_t i = 0; i < count; ++i) {
        ::new (p + i) T();
    }
    return p;
}

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
