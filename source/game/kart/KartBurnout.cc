#include "KartBurnout.hh"

#include "game/kart/KartPhysics.hh"
#include "game/kart/KartState.hh"

#include <egg/math/Math.hh>

namespace Kinoko::Kart {

/// @addr{inlined in 0x80577FC4}
KartBurnout::KartBurnout() = default;

/// @addr{0x805781DC}
KartBurnout::~KartBurnout() = default;

/// @addr{0x80589118}
/// @brief Calculates the burnout rotation and checks if the burnout has ended
void KartBurnout::calc() {
    constexpr u32 BURNOUT_DURATION = 120;

    if (!isActive()) {
        return;
    }

    calcRotation();

    if (calcEnd(BURNOUT_DURATION)) {
        deactivate();
    }
}

/// @addr{0x80589308}
/// @brief Calculates the rotation of the kart during a burnout
/// @details After 30 frames, applies a dampening effect to the rotation, reducing the amplitude of
/// the rotation over time.
void KartBurnout::calcRotation() {
    constexpr u16 PHASE_INCREMENT = 800;
    constexpr f32 PHASE_TO_FIDX = 1.0f / 256.0f;
    constexpr f32 AMPLITUDE_FACTOR = 40.0f;
    constexpr f32 DAMPENING_THRESHOLD = 30.0f;
    constexpr f32 DAMPENING_FACTOR = 0.97f;

    m_phase += PHASE_INCREMENT;

    f32 sin = EGG::Mathf::SinFIdx(static_cast<f32>(m_phase) * PHASE_TO_FIDX);

    // Apply a dampening effect after burning out for 30 frames
    if (static_cast<f32>(m_timer) > DAMPENING_THRESHOLD) {
        m_amplitude *= DAMPENING_FACTOR;
    }

    m_yaw = DEG2RAD * (AMPLITUDE_FACTOR * sin) * m_amplitude;

    physics()->composeStuntRot(EGG::Quatf::FromRPY(0.0f, m_yaw, 0.0f));
}

/// @addr{0x80589844}
/// @brief Sets the @enum eStatus::Burnout bit in the kart's status
void KartBurnout::activate() {
    status().setBit(eStatus::Burnout);
}

/// @addr{0x80589818}
/// @brief Resets the @enum eStatus::Burnout bit in the kart's status
void KartBurnout::deactivate() {
    status().resetBit(eStatus::Burnout);
}

/// @addr{0x80589830}
/// @brief Checks if the burnout is currently active
bool KartBurnout::isActive() const {
    return status().onBit(eStatus::Burnout);
}

} // namespace Kinoko::Kart
