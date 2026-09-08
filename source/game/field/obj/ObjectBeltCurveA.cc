#include "ObjectBeltCurveA.hh"

namespace Kinoko::Field {

/// @addr{0x807FC9D4}
/// @brief Calculates the conveyer belt's velocity at a given position based on its variant
/// @param variant The variant of the conveyer belt
/// @param pos The position at which to calculate the velocity
/// @param timeOffset Optional time delta
/// @return The velocity of the conveyer belt at the given position and variant
EGG::Vector3f ObjectBeltCurveA::calcRoadVelocity(u32 variant, const EGG::Vector3f &pos,
        u32 timeOffset) const {
    EGG::Vector3f posDelta = pos - this->pos();
    posDelta.y = 0.0f;

    EGG::Vector3f dir = m_initRt.ps_multVector(posDelta);
    bool forward = isMovingForward(timeOffset);

    if (variant == 4) {
        f32 sign = forward ? 1.0f : -1.0f;
        return dir * sign * calcDirSwitchSpeed(timeOffset);
    } else if (variant == 5) {
        f32 sign = forward ? -1.0f : 1.0f;
        return dir * sign * calcDirSwitchSpeed(timeOffset);
    } else {
        return EGG::Vector3f::zero;
    }
}

/// @addr{0x807FD5A0}
/// @brief Calculates the speed of the conveyor belt during a direction switch
/// @param t The current frame of the race
/// @return The speed of the conveyor belt at the given time
f32 ObjectBeltCurveA::calcDirSwitchSpeed(u32 t) const {
    s32 change1Delta = static_cast<s32>(t) - static_cast<s32>(m_dirChange1Frame);
    s32 change2Delta = static_cast<s32>(t) - static_cast<s32>(m_dirChange2Frame);
    u32 change1DeltaAbs = static_cast<u32>(EGG::Mathf::abs(static_cast<f32>(change1Delta)));
    u32 change2DeltaAbs = static_cast<u32>(EGG::Mathf::abs(static_cast<f32>(change2Delta)));
    u32 delta = std::min(change1DeltaAbs, change2DeltaAbs);

    if (static_cast<f32>(delta) > 60.0f) {
        return 0.006f;
    } else {
        return 0.006f * static_cast<f32>(delta) / 60.0f;
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
