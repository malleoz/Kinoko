#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief Calculates the duration of burnout and rotation induced
/// when holding acceleration too long during the race countdown.
class KartBurnout : private KartObjectProxy {
public:
    KartBurnout();
    ~KartBurnout() override;

    void start();
    void calc();

    /// @beginGetters

    [[nodiscard]] f32 yaw() const;

    /// @endGetters

private:
    void calcRotation();
    bool calcEnd(u32 duration);
    void activate();
    void deactivate();

    [[nodiscard]] bool isActive() const;

    u32 m_timer;     ///< The number of frames that have passed since the burnout started
    u16 m_phase;     ///< The phase of the burnout rotation, used to calculate the yaw
    f32 m_amplitude; ///< The amplitude of the burnout rotation, used to calculate the yaw
    f32 m_yaw;       ///< The current rotation of the kart due to the burnout
};

} // namespace Kinoko::Kart
