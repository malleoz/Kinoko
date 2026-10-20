#include "List.hh"

namespace Kinoko::Abstract::Memory {

/// @brief Constructor that initializes an empty list with a @ref m_offset of 0
MEMList::MEMList() : MEMList(0) {}

/// @addr{0x80199BF0}
/// @brief Constructor that initializes an empty list with the specified @ref m_offset
/// @param offset The offset to be used for the list
/// @details Initializes the head and tail to `nullptr`. Sets @ref m_numObjects to zero and @ref
/// m_offset to the provided `offset`.
MEMList::MEMList(u16 offset) : m_offset(offset) {
    m_headObject = nullptr;
    m_tailObject = nullptr;
    m_numObjects = 0;
}

/// @brief Default equality operator
/// @return `true` if the lists are equal, `false` otherwise
bool MEMList::operator==(const MEMList &) const = default;

/// @addr{0x80199C08}
/// @brief Appends an object to the end of the list
/// @param object The object to append
/// @warning Panics if `object` is `nullptr`.
void MEMList::append(void *object) {
    ASSERT(object);

    if (!m_headObject) {
        setFirstObject(object);
    } else {
        MEMLink *link = getLink(object);

        link->m_prevObject = m_tailObject;
        link->m_nextObject = nullptr;

        getLink(m_tailObject)->m_nextObject = object;
        m_tailObject = object;
        ++m_numObjects;
    }
}

/// @addr{0x80199C78}
/// @brief Removes an object from the list
/// @param object The object to remove
/// @warning Panics if `object` is `nullptr`.
void MEMList::remove(void *object) {
    ASSERT(object);

    MEMLink *link = getLink(object);

    if (!link->m_prevObject) {
        m_headObject = link->m_nextObject;
    } else {
        getLink(link->m_prevObject)->m_nextObject = link->m_nextObject;
    }

    if (!link->m_nextObject) {
        m_tailObject = link->m_prevObject;
    } else {
        getLink(link->m_nextObject)->m_prevObject = link->m_prevObject;
    }

    link->m_prevObject = nullptr;
    link->m_nextObject = nullptr;
    --m_numObjects;
}

/// @brief Retrieves the first object in the list
/// @return The first object, or `nullptr` if the list is empty
void *MEMList::getFirst() {
    return getNext(nullptr);
}

/// @addr{0x80199CE4}
/// @brief Retrieves the next object in the list
/// @param object The current object
/// @return The next object, or `nullptr` if `object` is the last one or the list is empty
void *MEMList::getNext(void *object) {
    return object ? getLink(object)->m_nextObject : m_headObject;
}

/// @brief Sets the first object in the list
/// @param object The object to set as the first
/// @warning Panics if `object` is `nullptr`.
void MEMList::setFirstObject(void *object) {
    ASSERT(object);

    MEMLink *link = getLink(object);

    link->m_nextObject = nullptr;
    link->m_prevObject = nullptr;

    m_headObject = object;
    m_tailObject = object;
    ++m_numObjects;
}

/// @brief Retrieves the link associated with an object
/// @param object The object whose link is to be retrieved
/// @return The link corresponding to the object
MEMLink *MEMList::getLink(void *object) {
    return reinterpret_cast<MEMLink *>(GetAddrNum(object) + m_offset);
}

} // namespace Kinoko::Abstract::Memory
