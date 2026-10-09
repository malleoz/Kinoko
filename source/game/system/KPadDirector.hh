#pragma once

#include "game/system/KPadController.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

/// @brief The highest level abstraction for controller input processing
class KPadDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @addr{0x805238F0}
    /// @brief Every frame, checks controller state and updates the player input state accordingly
    /// @details Calls @ref calcPads() to update the controller state, then updates the player's
    /// input state via @ref KPadPlayer::calc().
    void calc() {
        calcPads();
        m_playerInput.calc();
    }

    /// @addr{0x805237E8}
    /// @brief Updates the state of the ghost and host controllers
    /// @details Calls the `calc()` function for both @ref m_ghostController and @ref
    /// m_hostController.
    void calcPads() {
        m_ghostController->calc();
        m_hostController->calc();
    }

    /// @addr{0x80523724}
    /// @brief In the base game, this just clears motor rumble. For Kinoko, this is a no-op
    void clear() {}

    /// @addr{0x80523690}
    /// @brief Resets the player's current input state
    void reset() {
        m_playerInput.reset();
    }

    /// @addr{0x80524580}
    /// @brief Signals that the @ref KPadPlayer should begin reading from the ghost controller
    void startGhostProxies() {
        m_playerInput.startGhostProxy();
    }

    /// @addr{0x805245DC}
    /// @brief Signals that the @ref KPadPlayer should stop reading from the ghost controller (due
    /// to @ref Scene::RaceScene end)
    void endGhostProxies() {
        m_playerInput.endGhostProxy();
    }

    /// @addr{0x8052453C}
    /// @brief Assigns the ghost controller and its input data to the @ref KPadPlayer instance
    /// @param inputs The input data from the ghost controller
    /// @param driftIsAuto Whether drift is automatic
    void setGhostPad(const u8 *inputs, bool driftIsAuto) {
        m_playerInput.setGhostController(m_ghostController, inputs, driftIsAuto);
    }

    /// @brief Assigns the host controller to the @ref KPadPlayer instance
    /// @param driftIsAuto Whether drift is automatic
    void setHostPad(bool driftIsAuto) {
        m_playerInput.setHostController(m_hostController, driftIsAuto);
    }

    /// @beginGetters

    /// @brief Gets the player's pad
    /// @return A const ref to the player's pad, @ref m_playerInput
    [[nodiscard]] const KPadPlayer &playerInput() const {
        return m_playerInput;
    }

    /// @brief Gets the host controller
    /// @return A pointer to the host controller, @ref m_hostController
    [[nodiscard]] KPadHostController *hostController() {
        return m_hostController;
    }

    /// @endGetters

    /// @addr{0x8052313C}
    /// @brief Creates the singleton instance of the @ref KPadDirector
    /// @return A pointer to the newly created @ref KPadDirector instance
    /// @pre The singleton instance of the @ref KPadDirector does not already exist
    static KPadDirector *CreateInstance() {
        ASSERT(!s_instance);
        return s_instance = EGG::egg_new<KPadDirector>();
    }

    /// @addr{0x8052318C}
    /// @brief Destroys the singleton instance of the @ref KPadDirector
    /// @pre The singleton instance of the @ref KPadDirector must already exist
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref KPadDirector
    /// @return A pointer to the singleton instance of the @ref KPadDirector
    [[nodiscard]] static KPadDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    /// @addr{0x805232F0}
    /// @brief Private constructor that initializes the ghost and host controllers
    KPadDirector() {
        m_ghostController = EGG::egg_new<KPadGhostController>();
        m_hostController = EGG::egg_new<KPadHostController>();
    }

    /// @addr{0x805231DC}
    /// @brief Private destructor
    ~KPadDirector() override {
        if (s_instance) {
            s_instance = nullptr;
            WARN("KPadDirector instance not explicitly handled!");
        }
    }

    KPadPlayer m_playerInput;               ///< The player's pad state
    KPadGhostController *m_ghostController; ///< Controller object for ghost input playback
    KPadHostController *m_hostController;   ///< Controller object for external sources

    /// @addr{0x809BD70C}
    ///< @brief Singleton instance of the @ref KPadDirector
    static KPadDirector *s_instance;
};

} // namespace System

} // namespace Kinoko
