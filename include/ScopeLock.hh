#pragma once

#include <Common.hh>

#include <egg/core/ExpHeap.hh>

namespace Kinoko {

/// @brief A generic scope-based lock that temporarily changes a property or state
/// @tparam T The type of property or state that the scope lock will manage
/// @see @ref ScopeLock<GroupID>
template <typename T>
class ScopeLock;

/// @brief Temporarily changes the group ID of a given heap to better track memory allocation.
/// @details Typical pattern is `ScopeLock<GroupID> lock(groupID);` to initialize a lock.
template <>
class ScopeLock<GroupID> {
public:
    /// @brief Constructor
    /// @param newID The @ref GroupID to assign to the heap for the duration of the current scope
    ScopeLock(GroupID newID) {
        EGG::ExpHeap *heap = EGG::Heap::dynamicCastToExp(EGG::Heap::getCurrentHeap());
        ASSERT(heap);
        GroupID prevID = static_cast<GroupID>(heap->getGroupID());

        if (prevID != GroupID::None) {
            WARN("Overwriting non-default group ID! Replacing %d with %d", prevID, newID);
        }

        heap->setGroupID(static_cast<u16>(newID));
    }

    /// @brief Destructor that resets the @ref GroupID for the current heap to the default value
    ~ScopeLock() {
        EGG::ExpHeap *heap = EGG::Heap::dynamicCastToExp(EGG::Heap::getCurrentHeap());
        ASSERT(heap);
        heap->setGroupID(static_cast<u16>(GroupID::None));
    }
};

} // namespace Kinoko
