#include "JugemSwitch.hh"

#include "game/kart/KartObjectManager.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x80725C98}
/// @details Checks if the player is driving the wrong way and updates the internal state of the
/// switch accordingly. If the player is driving the wrong way for 60 frames, Lakitu will toggle on.
void JugemSwitchReverse::calc() {
    constexpr f32 ACTIVATION_FRAME_STEP = 1.0f / 60.0f;

    if (System::RaceManager::Instance()->player().drivingWrongWay()) {
        const auto &kartStatus = Kart::KartObjectManager::Instance()->object(0)->status();
        if (kartStatus.offBit(Kart::eStatus::InAction)) {
            m_activationPercent += ACTIVATION_FRAME_STEP;
        }

        if (m_activationPercent > 1.0f && kartStatus.offBit(Kart::eStatus::BeforeRespawn)) {
            m_activationPercent = 1.0f;
            m_isOn = true;
        }
    } else {
        m_activationPercent -= ACTIVATION_FRAME_STEP;
        if (m_activationPercent < 0.0f) {
            m_activationPercent = 0.0f;
            m_isOn = false;
        }
    }
}

} // namespace Kinoko::Field
