#pragma once

#include "abstract/memory/Memory.hh"

namespace Kinoko::Abstract::Memory {

/// @brief Link of an intrusive doubly-linked list.
struct MEMLink {
    void *m_prevObject;
    void *m_nextObject;
};

/// @brief Intrusive doubly-linked list. Links are placed within the corresponding object.
/// @details This implementation is better than using `std::list` due to the following reasons:
/// 1. This implementation never performs allocation when adding or removing elements, as the links
/// are embedded within the objects themselves.
/// 2. This implementation is safer for savestates. @ref Host::Context can `memcpy` this
/// implementation, whereas `std-list` has implementation-defined internals which may not work
/// across sessions or machines.
/// 3. Removals are O(1). `std::list` requires iterating through the list which is O(n).
struct MEMList {
    MEMList();
    MEMList(u16 offset);

    bool operator==(const MEMList &rhs) const = default;

    void append(void *object);
    void remove(void *object);
    [[nodiscard]] void *getFirst();
    [[nodiscard]] void *getNext(void *object);

    void *m_headObject;
    void *m_tailObject;
    u16 m_numObjects;
    u16 m_offset;

private:
    void setFirstObject(void *object);
    [[nodiscard]] MEMLink *getLink(void *object);
};

} // namespace Kinoko::Abstract::Memory
