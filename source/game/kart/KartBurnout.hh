#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief Calculates the duration of burnout and rotation induced
/// when holding acceleration too long during the race countdown.
class KartBurnout : KartObjectProxy {
public:
    KartBurnout();
    ~KartBurnout();

    /// @addr{0x805890B0}
    /// @brief Runs when a burnout occurs at the beginning of the race
    void start() {
        activate();
        m_timer = 0;
        m_phase = 0;
        m_amplitude = 1.0f;
    }

    void calc();

    /// @beginGetters
    [[nodiscard]] f32 pitch() const {
        return m_yaw;
    }
    /// @endGetters

private:
    void calcRotation();

    /// @addr{0x8058920C}
    /// @brief Checks if the burnout duration has ended
    /// @param duration The duration of the burnout in frames
    bool calcEnd(u32 duration) {
        return ++m_timer >= duration;
    }

    void activate();
    void deactivate();
    [[nodiscard]] bool isActive() const;

    u32 m_timer;     ///< The number of frames that have passed since the burnout started
    u16 m_phase;     ///< The phase of the burnout rotation, used to calculate the yaw
    f32 m_amplitude; ///< The amplitude of the burnout rotation, used to calculate the yaw
    f32 m_yaw;       ///< The current rotation of the kart due to the burnout
};

} // namespace Kinoko::Kart
