#include "JugemDirector.hh"

#include "game/kart/KartObjectManager.hh"

namespace Kinoko::Field {

/// @addr{0x8071E330}
/// @brief Private constructor
JugemDirector::JugemDirector() : m_unit(nullptr) {}

/// @addr{0x8071E390}
/// @brief Private destructor that destroys the underlying @ref JugemUnit
JugemDirector::~JugemDirector() {
    EGG::egg_delete(m_unit);
}

/// @addr{0x8071E480}
/// @brief Constructs a @ref JugemUnit for the player and its associated @ref JugemSwitch instances
void JugemDirector::createUnits() {
    // Assumes one unit
    const auto *kartObj = Kart::KartObjectManager::Instance()->object(0);
    m_unit = EGG::egg_new<JugemUnit>(kartObj);

    m_unit->createSwitchRace();
}

JugemDirector *JugemDirector::s_instance = nullptr;

} // namespace Kinoko::Field
