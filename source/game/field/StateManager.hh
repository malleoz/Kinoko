#pragma once

#include "game/field/obj/ObjectBase.hh"

namespace Kinoko::Field {

/// @brief Represents a single entry in the StateManager's state table
/// @details Each entry contains an ID, a function pointer invoked when the state is entered, and a
/// function pointer invoked every frame while the state is active
struct StateManagerEntry {
    u16 id;                  ///< The ID of the state, always maps to the index in the entry table
    void (*onEnter)(void *); ///< Function pointer invoked when the state is entered
    void (*onCalc)(void *);  ///< Function pointer invoked every frame while the state is active
};

/// @brief Templated helper function to create a StateManagerEntry for an object of type T
/// @tparam T The type of the object that will use this state entry
/// @tparam Enter Pointer to the member function of T, called when the state is entered
/// @tparam Calc Pointer to the member function of T, called every frame while the state is active
/// @param id The ID of the state, always maps to the index in the entry table
/// @return A StateManagerEntry struct initialized with the provided ID and function pointers
template <typename T, void (T::*Enter)(), void (T::*Calc)()>
constexpr StateManagerEntry StateEntry(u16 id) {
    auto enter = [](void *obj) { (reinterpret_cast<T *>(obj)->*Enter)(); };
    auto calc = [](void *obj) { (reinterpret_cast<T *>(obj)->*Calc)(); };
    return {id, enter, calc};
}

/// @brief Base class that manages different "states" for an object
/// @details Objects can inherit from this class in order to define unique behavior depending on
/// what the current "state" of the object is. For example, @ref ObjectFireSnake may be falling from
/// the sun, resting, or jumping. Each state has an "enter" and a "calc" function. The object that
/// inherits this class is responsible for defining how many states there are and must specify the
/// enter and calc functions for each of those states, using the @ref StateEntry helper function.
/// The object is also responsible for defining the transition points (i.e. the criteria to move
/// from one state to another) and updating the next state ID accordingly.
/// @note This class was originally implemented using the curriously recurring template pattern
/// (CRTP). However, we ran into inheritance issues where a subclass of an object needed to define
/// its own member function pointers for enter and calc, but the template type T in the StateManager
/// no longer matched the subclass type (since the parent class inherits from @ref StateManager).
/// Rather than try to leverage virtual inheritance with wrapper functions to call the parent member
/// functions, we instead refactored to use @ref StateManagerEntry structs each having lambdas which
/// invoke the enter and calc functions.
class StateManager {
protected:
    /// @brief Constructor that initializes the @ref StateManager with the object that owns it and
    /// the state entries
    /// @param obj The object that owns this @ref StateManager instance
    /// @param entries A span of @ref StateManagerEntry structs that define the set of enter and
    /// calc functions for each state Id
    StateManager(void *obj, const std::span<const StateManagerEntry> &entries)
        : m_currentStateId(0), m_nextStateId(-1), m_currentFrame(0), m_entryIds(entries.size()),
          m_entries(entries), m_obj(obj) {
        // The base game initializes all entries to 0xffff, possibly to avoid an uninitialized value
        memset(m_entryIds.begin(), 0xff, m_entryIds.size());

        for (size_t i = 0; i < m_entryIds.size(); ++i) {
            m_entryIds[m_entries[i].id] = i;
        }
    }

    /// @brief Default virtual destructor
    virtual ~StateManager() = default;

    /// @brief Evaluates the current state and invokes the appropriate enter and calc functions
    /// @details If the inheriting object has set the next state ID to a valid value, transitions to
    /// that state by invoking the onEnter function and resetting the current frame counter and next
    /// state ID. Otherwise, increments the current frame counter. In both scenarios, invokes the
    /// onCalc function for the current state.
    void calc() {
        if (m_nextStateId >= 0) {
            m_currentStateId = m_nextStateId;
            m_nextStateId = -1;
            m_currentFrame = 0;

            auto enterFunc = m_entries[m_entryIds[m_currentStateId]].onEnter;
            enterFunc(m_obj);
        } else {
            ++m_currentFrame;
        }

        auto calcFunc = m_entries[m_entryIds[m_currentStateId]].onCalc;
        calcFunc(m_obj);
    }

    u16 m_currentStateId; ///< ID of the current state
    s32 m_nextStateId;    ///< ID of the state to transition to, or -1 if no transition is requested
    u32 m_currentFrame; ///< Number of frames that have elapsed since the current state was entered
    owning_span<u16> m_entryIds; ///< Maps state IDs to indices in the m_entries span

    /// @brief Span of state entries defining the enter and calc functions for each state
    const std::span<const StateManagerEntry> m_entries;

    void *m_obj; ///< Pointer to the object that owns this @ref StateManager instance
};

} // namespace Kinoko::Field
