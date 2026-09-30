#include "ObjectBeltCurveA.hh"

namespace Kinoko::Field {

/// @addr{0x807FC9D4}
/// @brief Calculates the conveyer belt's velocity at a given position based on its variant
/// @param variant The variant of the conveyer belt
/// @param pos The position at which to calculate the velocity
/// @param timeOffset Optional time delta
/// @return The velocity of the conveyer belt at the given position and variant
/// @details If `variant` is 4, then this represents the inner conveyer belt that starts reversed,
/// otherwise it is the outer conveyer belt and starts forward. The sign of the velocity is
/// determined accordingly. Computes the kart's tangent direction scaled by its distance from the
/// conveyer belt's origin (the kart will move faster the closer it is to the outer edge of the
/// conveyer belt). Finally, applies a speed scalar that factors in the conveyer belt switching
/// directions.
EGG::Vector3f ObjectBeltCurveA::calcRoadVelocity(u32 variant, const EGG::Vector3f &pos,
        u32 timeOffset) const {
    EGG::Vector3f posDelta = pos - this->pos();
    posDelta.y = 0.0f;

    EGG::Vector3f tanVel = m_initRt.ps_multVector(posDelta);
    bool forward = isMovingForward(timeOffset);

    if (variant == 4) {
        f32 sign = forward ? 1.0f : -1.0f;
        return tanVel * sign * calcAngularSpeed(timeOffset);
    } else if (variant == 5) {
        f32 sign = forward ? -1.0f : 1.0f;
        return tanVel * sign * calcAngularSpeed(timeOffset);
    } else {
        return EGG::Vector3f::zero;
    }
}

/// @addr{0x807FD5A0}
/// @brief Calculates the angular speed of the conveyor belt, including during a direction switch
/// @param t The current frame of the race
/// @return The angular speed (in radians per frame)of the conveyor belt at the given time
/// @details If the conveyer belt is not changing direction, returns `0.006f`. For 60 frames before
/// and after the slowdown, the angular speed gradually decreases or increases linearly.
/// @note To induce a slowdown before and after a direction change, this function handles the case
/// where the delta time for a given direction switch is negative. This way, the conveyer belt slows
/// down for 60 frames before the direction switch, and speeds up for 60 frames after the direction
/// switch.
f32 ObjectBeltCurveA::calcAngularSpeed(u32 t) const {
    constexpr f32 ACCEL_FRAMES = 60.0f;
    constexpr f32 DEFAULT_ANG_SPEED = 0.006f;

    s32 change1Delta = static_cast<s32>(t) - static_cast<s32>(m_dirChange1Frame);
    s32 change2Delta = static_cast<s32>(t) - static_cast<s32>(m_dirChange2Frame);
    u32 change1DeltaAbs = static_cast<u32>(EGG::Mathf::abs(static_cast<f32>(change1Delta)));
    u32 change2DeltaAbs = static_cast<u32>(EGG::Mathf::abs(static_cast<f32>(change2Delta)));
    u32 delta = std::min(change1DeltaAbs, change2DeltaAbs);

    if (static_cast<f32>(delta) > ACCEL_FRAMES) {
        return DEFAULT_ANG_SPEED;
    } else {
        return DEFAULT_ANG_SPEED * static_cast<f32>(delta) / ACCEL_FRAMES;
    }
}

/// @addr{0x807FD66C}
/// @brief Based on the provided time, checks whether the belt is moving forward or backward
/// @param t The current frame of the race
/// @return `true` if the conveyor belt is moving forward at the given time, `false` otherwise
bool ObjectBeltCurveA::isMovingForward(u32 t) const {
    if (t <= m_dirChange1Frame) {
        return m_startForward;
    }

    if (t <= m_dirChange2Frame) {
        return !m_startForward;
    }

    return m_startForward;
}

} // namespace Kinoko::Field
