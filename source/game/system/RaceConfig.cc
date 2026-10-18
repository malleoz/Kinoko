#include "RaceConfig.hh"

#include "game/system/KPadDirector.hh"

namespace Kinoko::System {

/// @addr{Inlined in 0x8052DD40}
/// @brief Initializes the scenario data for the race
/// @details The base game sets the course to @ref Course::GCN_Mario_Circuit and all players to
/// @ref Character::Mario with @ref Vehicle::Standard_Kart_M.
/// @todo We should be able to skip this for Kinoko, since we don't rely on the default
/// initialization.
void RaceConfig::Scenario::init() {
    playerCount = 0;
    course = Course::GCN_Mario_Circuit;

    for (size_t i = 0; i < players.size(); ++i) {
        Player &player = players[i];
        player.character = Character::Mario;
        player.vehicle = Vehicle::Standard_Kart_M;
        player.type = Player::Type::None;
    }
}

/// @addr{0x8052DD40}
/// @brief Initializes the @ref Scenario data for the race
/// @details Simply calls @ref Scenario::init() on @ref m_raceScenario.
void RaceConfig::init() {
    m_raceScenario.init();
}

/// @addr{0x805302C4}
/// @brief Initialization that occurs upon loading a race
/// @details Sets the race scenario's player count to 1 since we only implement a single player in
/// Kinoko. Normally we copy the menu scenario into the race scenario. However, Kinoko doesn't
/// support menus, so we use a callback. Finally, initializes controllers via @ref
/// initControllers().
void RaceConfig::initRace() {
    m_raceScenario.playerCount = 1;

    if (s_onInitCallback) {
        s_onInitCallback(this, s_onInitCallbackArg);
    }

    initControllers();
}

/// @addr{0x8052F4E8}
/// @brief Initializes the controllers for each player
/// @details If the player is a ghost, then calls @ref initGhost(). If the player is a local/live
/// player, then initializes the controller via @ref KPadDirector::setHostPad(). Otherwise, panics
/// due to an invalid player type.
/// @note This is normally scoped within RaceConfig::Scenario, but Kinoko doesn't support menus.
/// For Kinoko, we just care about the player at index 0.
void RaceConfig::initControllers() {
    switch (m_raceScenario.players[0].type) {
    case Player::Type::Ghost:
        initGhost();
        break;
    case Player::Type::Local:
        KPadDirector::Instance()->setHostPad(m_raceScenario.players[0].driftIsAuto);
        break;
    default:
        PANIC("Players must be either local or ghost!");
        break;
    }
}

/// @addr{0x8052EEF0}
/// @brief Initializes the ghost
/// @details Sets the race scenario's course and player info based on the ghost data. Finally, sets
/// the ghost pad via @ref KPadDirector::setGhostPad().
/// @note This is normally scoped within RaceConfig::Scenario, but Kinoko doesn't support menus.
void RaceConfig::initGhost() {
    GhostFile ghost(m_ghost);

    m_raceScenario.course = ghost.course();
    Player &player = m_raceScenario.players[0];
    player.character = ghost.character();
    player.vehicle = ghost.vehicle();
    player.driftIsAuto = ghost.driftIsAuto();

    KPadDirector::Instance()->setGhostPad(ghost.inputs(), ghost.driftIsAuto());
}

/// @brief Sets the ghost data for the race
/// @param rkg Pointer to the raw (possibly uncompressed) ghost `.rkg` file buffer
/// @details Creates a @ref RawGhostFile instance from the provided raw ghost file buffer and
/// stores it in @ref m_ghost.
void RaceConfig::setGhost(const u8 *rkg) {
    m_ghost = RawGhostFile(rkg);
}

/// @brief Gets the current race scenario
/// @return A non-const ref to the current race scenario, @ref m_raceScenario
RaceConfig::Scenario &RaceConfig::raceScenario() {
    return m_raceScenario;
}

/// @brief Gets the current race scenario
/// @return A const ref to the current race scenario, @ref m_raceScenario
const RaceConfig::Scenario &RaceConfig::raceScenario() const {
    return m_raceScenario;
}

/// @brief Sets the initialization callback for the RaceConfig instance
/// @param callback The callback function to call after initialization
/// @param arg The argument(s) to pass to the callback function
void RaceConfig::RegisterInitCallback(const InitCallback &callback, void *arg) {
    s_onInitCallback = callback;
    s_onInitCallbackArg = arg;
}

/// @addr{0x8052FE58}
/// @brief Creates the singleton instance of the @ref RaceConfig
/// @return A pointer to the newly created @ref RaceConfig instance
/// @pre The singleton instance of the RaceConfig must not already exist.
RaceConfig *RaceConfig::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<RaceConfig>();
    return s_instance;
}

/// @addr{0x8052FFE8}
/// @brief Destroys the singleton instance of the @ref RaceConfig
/// @pre The singleton instance of the RaceConfig must already exist.
void RaceConfig::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Returns the singleton instance of the @ref RaceConfig
/// @return A pointer to the singleton instance of the @ref RaceConfig
RaceConfig *RaceConfig::Instance() {
    return s_instance;
}

/// @addr{0x8053015C}
/// @brief Private default constructor
RaceConfig::RaceConfig() = default;

/// @addr{0x80530038}
/// @brief Private virtual destructor
RaceConfig::~RaceConfig() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("RaceConfig instance not explicitly handled!");
    }
}

RaceConfig *RaceConfig::s_instance = nullptr;
RaceConfig::InitCallback RaceConfig::s_onInitCallback = nullptr;
void *RaceConfig::s_onInitCallbackArg = nullptr;

} // namespace Kinoko::System
