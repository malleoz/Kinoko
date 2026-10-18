#include "KPadDirector.hh"

namespace Kinoko::System {

/// @addr{0x805238F0}
/// @brief Every frame, checks controller state and updates the player input state accordingly
/// @details Calls @ref calcPads() to update the controller state, then updates the player's
/// input state via @ref KPadPlayer::calc().
void KPadDirector::calc() {
    calcPads();
    m_playerInput.calc();
}

/// @addr{0x805237E8}
/// @brief Updates the state of the ghost and host controllers
/// @details Calls the `calc()` function for both @ref m_ghostController and @ref
/// m_hostController.
void KPadDirector::calcPads() {
    m_ghostController->calc();
    m_hostController->calc();
}

/// @addr{0x80523724}
/// @brief In the base game, this just clears motor rumble. For Kinoko, this is a no-op
void KPadDirector::clear() {}

/// @addr{0x80523690}
/// @brief Resets the player's current input state
void KPadDirector::reset() {
    m_playerInput.reset();
}

/// @addr{0x80524580}
/// @brief Signals that the @ref KPadPlayer should begin reading from the ghost controller
void KPadDirector::startGhostProxies() {
    m_playerInput.startGhostProxy();
}

/// @addr{0x805245DC}
/// @brief Signals that the @ref KPadPlayer should stop reading from the ghost controller (due
/// to @ref Scene::RaceScene end)
void KPadDirector::endGhostProxies() {
    m_playerInput.endGhostProxy();
}

/// @addr{0x8052453C}
/// @brief Assigns the ghost controller and its input data to the @ref KPadPlayer instance
/// @param inputs The input data from the ghost controller
/// @param driftIsAuto Whether drift is automatic
void KPadDirector::setGhostPad(const u8 *inputs, bool driftIsAuto) {
    m_playerInput.setGhostController(m_ghostController, inputs, driftIsAuto);
}

/// @brief Assigns the host controller to the @ref KPadPlayer instance
/// @param driftIsAuto Whether drift is automatic
void KPadDirector::setHostPad(bool driftIsAuto) {
    m_playerInput.setHostController(m_hostController, driftIsAuto);
}

/// @brief Gets the player's pad
/// @return A const ref to the player's pad, @ref m_playerInput
const KPadPlayer &KPadDirector::playerInput() const {
    return m_playerInput;
}

/// @brief Gets the host controller
/// @return A pointer to the host controller, @ref m_hostController
KPadHostController *KPadDirector::hostController() {
    return m_hostController;
}

/// @addr{0x8052313C}
/// @brief Creates the singleton instance of the @ref KPadDirector
/// @return A pointer to the newly created @ref KPadDirector instance
/// @pre The singleton instance of the @ref KPadDirector does not already exist
KPadDirector *KPadDirector::CreateInstance() {
    ASSERT(!s_instance);
    return s_instance = EGG::egg_new<KPadDirector>();
}

/// @addr{0x8052318C}
/// @brief Destroys the singleton instance of the @ref KPadDirector
/// @pre The singleton instance of the @ref KPadDirector must already exist
void KPadDirector::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Returns the singleton instance of the @ref KPadDirector
/// @return A pointer to the singleton instance of the @ref KPadDirector
KPadDirector *KPadDirector::Instance() {
    return s_instance;
}

/// @addr{0x805232F0}
/// @brief Private constructor that initializes the ghost and host controllers
KPadDirector::KPadDirector() {
    m_ghostController = EGG::egg_new<KPadGhostController>();
    m_hostController = EGG::egg_new<KPadHostController>();
}

/// @addr{0x805231DC}
/// @brief Private destructor
KPadDirector::~KPadDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KPadDirector instance not explicitly handled!");
    }
}

KPadDirector *KPadDirector::s_instance = nullptr;

} // namespace Kinoko::System
