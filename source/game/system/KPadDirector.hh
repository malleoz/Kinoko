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
    void calc() {
        calcPads();
        m_playerInput.calc();
    }

    /// @addr{0x805237E8}
    /// @brief Updates the state of the ghost and host controllers
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
    /// @brief Signals that the @ref KPadPlayer should begin reading from the ghost
    /// controller
    void startGhostProxies() {
        m_playerInput.startGhostProxy();
    }

    /// @addr{0x805245DC}
    /// @brief Signals that the @ref KPadPlayer should stop reading from the ghost
    /// controller (due to @ref Scene::RaceScene end)
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
    [[nodiscard]] const KPadPlayer &playerInput() const {
        return m_playerInput;
    }

    [[nodiscard]] KPadHostController *hostController() {
        return m_hostController;
    }
    /// @endGetters

    /// @addr{0x8052313C}
    /// @brief Creates the singleton instance of the @ref KPadDirector
    /// @return A pointer to the newly created @ref KPadDirector instance
    static KPadDirector *CreateInstance() {
        ASSERT(!s_instance);
        return s_instance = EGG::egg_new<KPadDirector>();
    }

    /// @addr{0x8052318C}
    /// @brief Destroys the singleton instance of the @ref KPadDirector
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

    KPadDirector();
    ~KPadDirector() override;

    KPadPlayer m_playerInput;               ///< The player's pad state
    KPadGhostController *m_ghostController; ///< Controller object for ghost input playback
    KPadHostController *m_hostController;   ///< Controller object for external sources

    static KPadDirector *s_instance; ///< @addr{0x809BD70C}
};

} // namespace System

} // namespace Kinoko
