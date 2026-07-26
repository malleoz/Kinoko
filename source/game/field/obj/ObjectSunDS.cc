#include "ObjectSunDS.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x806DDDD8}
ObjectSunDS::ObjectSunDS(const System::MapdataGeoObj &params)
    : ObjectProjectileLauncher(params), StateManager(this, STATE_ENTRIES),
      m_revolutionSpeed(static_cast<f32>(params.setting(0))),
      m_startFrame(static_cast<s32>(params.setting(1))) {}

/// @addr{0x806DDF68}
ObjectSunDS::~ObjectSunDS() = default;

/// @addr{0x806DE03C}
/// @details Updates the sun's position along its rail. If the sun reaches a stop point, it
/// transitions to the still state.
void ObjectSunDS::calc() {
    if (System::RaceManager::Instance()->timer() < static_cast<u32>(m_startFrame)) {
        return;
    }

    calcRail();
    StateManager::calc();
    calcPos();
}

/// @addr{0x806DE598}
/// @brief If a projectile should be launched this frame, returns the corresponding rail point
/// index. Otherwise, returns -1.
s16 ObjectSunDS::launchPointIdx() {
    constexpr u16 THROW_DELAY = 30;

    if (m_currentStateId != 0 || THROW_DELAY != m_currentFrame) {
        return -1;
    }

    return m_railInterpolator->curPointIdx();
}

/// @addr{0x806DE458}
/// @brief Updates the sun's position along its rail and handles stop points
void ObjectSunDS::calcRail() {
    if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
        m_railInterpolator->setT(0.0f);
        checkStop();
    }
}

} // namespace Kinoko::Field
