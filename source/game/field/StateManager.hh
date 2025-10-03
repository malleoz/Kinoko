#pragma once

#include "game/field/obj/ObjectBase.hh"

#include <type_traits>

namespace Field {

struct StateManagerEntry {
    u16 id;
    void (*onEnter)(void *);
    void (*onCalc)(void *);
};

template <typename T, void (T::*Enter)(), void (T::*Calc)()>
constexpr StateManagerEntry StateEntry(u16 id) {
    auto enter = [](void *obj) { (static_cast<T *>(obj)->*Enter)(); };
    auto calc = [](void *obj) { (static_cast<T *>(obj)->*Calc)(); };
    return {id, enter, calc};
}

template <typename T>
class StateManager {
public:
    StateManager();

    virtual ~StateManager() {
        delete[] m_entryIds.data();
    }

protected:
    void calc(this T &self) {
        if (self.m_nextStateId >= 0) {
            self.m_currentStateId = self.m_nextStateId;
            self.m_nextStateId = -1;
            self.m_currentFrame = 0;

            auto enterFunc = self.m_entries[self.m_entryIds[self.m_currentStateId]].onEnter;
            enterFunc(&self);
        } else {
            ++self.m_currentFrame;
        }

        auto calcFunc = self.m_entries[self.m_entryIds[self.m_currentStateId]].onCalc;
        calcFunc(&self);
    }

    u16 m_currentStateId;
    s32 m_nextStateId;
    u32 m_currentFrame;
    std::span<u16> m_entryIds;
    std::span<const StateManagerEntry> m_entries;
};

/// @brief Defined outside of the class declaration so that typename T will be a complete type.
template <typename T>
StateManager<T>::StateManager() : m_currentStateId(0), m_nextStateId(-1), m_currentFrame(0) {
    // Concepts don't work here due to CRTP causing incomplete class type use.
    STATIC_ASSERT((std::is_base_of_v<ObjectBase, T>));
    STATIC_ASSERT((std::is_same_v<decltype(T::STATE_ENTRIES),
            const std::array<StateManagerEntry, T::STATE_ENTRIES.size()>>));

    m_entryIds = std::span(new u16[T::STATE_ENTRIES.size()], T::STATE_ENTRIES.size());

    // The base game initializes all entries to 0xffff, possibly to avoid an uninitialized value
    for (auto &id : m_entryIds) {
        id = 0xffff;
    }

    for (size_t i = 0; i < m_entryIds.size(); ++i) {
        m_entryIds[T::STATE_ENTRIES[i].id] = i;
    }

    m_entries = std::span<const StateManagerEntry>(T::STATE_ENTRIES);
}

} // namespace Field
