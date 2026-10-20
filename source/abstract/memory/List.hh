#pragma once

#include "abstract/memory/Memory.hh"

namespace Kinoko::Abstract::Memory {

/// @brief Link of an intrusive doubly-linked list.
/// @see MEMList
struct MEMLink {
    void *m_prevObject; ///< The previous object in the list (or `nullptr` is this is the head)
    void *m_nextObject; ///< The next object in the list (or `nullptr` if this is the tail)
};

/// @brief Intrusive doubly-linked list. Links are placed within the corresponding object.
/// @details This implementation is better than using `std::list` due to the following reasons:
/// 1. This implementation never performs allocation when adding or removing elements, as the links
/// are embedded within the objects themselves. Objects must contain a @ref MEMLink member at the
/// offset specified by @ref m_offset.
/// 2. This implementation is safer for savestates. @ref Host::Context can `memcpy` this
/// implementation, whereas `std-list` has implementation-defined internals which may not work
/// across sessions or machines.
/// 3. Removals are O(1). `std::list` requires iterating through the list which is O(n).
struct MEMList {
    MEMList();
    MEMList(u16 offset);

    bool operator==(const MEMList &) const;

    void append(void *object);
    void remove(void *object);

    /// @beginGetters

    [[nodiscard]] void *getFirst();
    [[nodiscard]] void *getNext(void *object);

    /// @endGetters

    void *m_headObject; ///< The object at the head of the list
    void *m_tailObject; ///< The object at the tail of the list
    u16 m_numObjects;   ///< The number of objects in the list
    u16 m_offset;       ///< The offset of the @ref MEMLink within the objects

private:
    void setFirstObject(void *object);
    [[nodiscard]] MEMLink *getLink(void *object);
};

} // namespace Kinoko::Abstract::Memory
