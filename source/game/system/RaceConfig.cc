#include "RaceConfig.hh"

#include "game/system/KPadDirector.hh"

namespace Kinoko::System {

/// @addr{0x805302C4}
/// @brief Initialization that occurs upon loading a race
/// @details Normally we copy the menu scenario into the race scenario.
/// However, Kinoko doesn't support menus, so we use a callback.
void RaceConfig::initRace() {
    m_raceScenario.playerCount = 1;

    if (s_onInitCallback) {
        s_onInitCallback(this, s_onInitCallbackArg);
    }

    initControllers();
}

/// @addr{0x8052F4E8}
/// @brief Initializes the controllers for each player
/// @details This is normally scoped within RaceConfig::Scenario, but Kinoko doesn't support menus.
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
/// @details This is normally scoped within RaceConfig::Scenario, but Kinoko doesn't support menus.
void RaceConfig::initGhost() {
    GhostFile ghost(m_ghost);

    m_raceScenario.course = ghost.course();
    Player &player = m_raceScenario.players[0];
    player.character = ghost.character();
    player.vehicle = ghost.vehicle();
    player.driftIsAuto = ghost.driftIsAuto();

    KPadDirector::Instance()->setGhostPad(ghost.inputs(), ghost.driftIsAuto());
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

RaceConfig *RaceConfig::s_instance = nullptr;
RaceConfig::InitCallback RaceConfig::s_onInitCallback = nullptr;
void *RaceConfig::s_onInitCallbackArg = nullptr;

} // namespace Kinoko::System
