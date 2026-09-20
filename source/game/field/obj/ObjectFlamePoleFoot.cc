#include "ObjectFlamePoleFoot.hh"

#include "game/field/CollisionDirector.hh"

#include <algorithm>

namespace Kinoko::Field {

/// @addr{0x8067E6F4}
/// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
/// @details Increments the static @ref s_flamePoleCount. @ref m_maxScale is initialized based on
/// the third param setting. If it is `0`, then the scale is instead computed as `3 +
/// (s_flamePoleCount % 3)`. Constructs and loads the associated @ref ObjectFlamePole object.
ObjectFlamePoleFoot::ObjectFlamePoleFoot(const System::MapdataGeoObj &params)
    : ObjectKCL(params),
      StateManager(this, STATE_ENTRIES),
      m_extraCycleFrames(params.setting(0)),
      m_initDelay(params.setting(1)) {
    m_maxScale = static_cast<f32>(params.setting(2));

    ++s_flamePoleCount;

    if (m_maxScale == 0.0f) {
        m_maxScale = 3.0f + static_cast<f32>(s_flamePoleCount % 3);
    }

    m_pole = EGG::egg_new<ObjectFlamePole>(params, pos(), rot(), scale());
    m_pole->load();
}

/// @addr{0x8067EC94}
/// @copybrief ObjectBase::init()
/// @details Even though this function sets @ref m_nextStateId to 0 (the expanding state), @ref
/// calc() will not actually evaluate the state machine until @ref m_initDelay frames elapse.
/// Calculates the velocity of the pole when it descends from an eruption. Calculates the height of
/// the fully erupted pole, as well as the @ref m_scaleDelta. Disables the pole's collision and sets
/// it to inactive. Finally, offsets the pole's position so that it is positioned below the geyser.
void ObjectFlamePoleFoot::init() {
    m_nextStateId = 0;
    m_cycleFrame = 0;

    m_eruptDownVel = (m_maxScale - 1.0f) / static_cast<f32>(STATE_STARTS[1]);
    m_maxHeight = ObjectFlamePole::HEIGHT * m_maxScale;
    m_scaleDelta = (300.0f + m_maxHeight) / static_cast<f32>(ERUPT_DOWN_DURATION);
    m_pole->setActive(false);
    m_pole->disableCollision();

    EGG::Vector3f polePos = m_pole->pos();
    polePos.y = pos().y - ObjectFlamePole::HEIGHT * m_maxScale;
    m_pole->setPos(polePos);

    initEruptKinematics();
}

/// @addr{0x8067FC50}
/// @copybrief ObjectKCL::getScaleY()
/// @param timeOffset The time offset used to calculate the current frame's scale
/// @return The current scale of the geyser hump.
/// @details If @ref m_initDelay frames have not elapsed in the race yet, then returns `1.0f`. If
/// the geyser is dormant, then returns `1.0f`. If the geyser is erupting, then computes the scale
/// based off how many frames have elapsed since the start of that state. If the geyser is finished
/// erupting, then the geyser's scale starts decreating halfway through the erupting down state. If
/// the geyser is fully erupted, then the scale remains at @ref m_maxScale.
f32 ObjectFlamePoleFoot::getScaleY(u32 timeOffset) const {
    u32 frame = System::RaceManager::Instance()->timer() - timeOffset;
    if (frame < m_initDelay) {
        return 1.0f;
    }

    s32 cycleFrame = (frame - m_initDelay) % (m_extraCycleFrames + CYCLE_DURATION);

    if (cycleFrame >= STATE_STARTS[5]) {
        return 1.0f;
    }

    if (cycleFrame >= STATE_STARTS[4]) {
        return std::max(1.0f,
                m_maxScale - m_eruptDownVel * static_cast<f32>(ERUPT_DOWN_DURATION / 2) -
                        m_eruptDownVel * static_cast<f32>(cycleFrame - STATE_STARTS[4]));
    }

    if (cycleFrame >= STATE_STARTS[3]) {
        s32 framesFalling = cycleFrame - STATE_STARTS[3];
        s32 halfDuration = ERUPT_DOWN_DURATION / 2;
        if (framesFalling > halfDuration) {
            return m_maxScale - m_eruptDownVel * static_cast<f32>(framesFalling - halfDuration);
        } else {
            return m_maxScale;
        }
    }

    if (cycleFrame >= STATE_STARTS[2] || cycleFrame >= STATE_STARTS[1]) {
        return m_maxScale;
    }

    if (cycleFrame >= 0) {
        return std::min(m_maxScale, m_eruptDownVel * static_cast<f32>(cycleFrame) + 1.0f);
    }

    return 1.0f;
}

/// @addr{0x8067FE88}
/// @copydoc ObjectKCL::checkCollision()
/// @details Updates the @ref ObjColMgr transform and scale. Dispatches to the @ref ObjColMgr to
/// check for collision. Humps become trickable once the geyser's scale is 2 or greater.
bool ObjectFlamePoleFoot::checkCollision(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);
    f32 scale = getScaleY(timeOffset);

    if (!m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut)) {
        return false;
    }

    if (2.0f < scale) {
        CollisionDirector::Instance()->setCurrentCollisionTrickable(true);
    }

    return true;
}

/// @addr{0x80680218}
/// @copydoc ObjectKCL::checkCollisionCached()
/// @details Updates the @ref ObjColMgr transform and scale. Dispatches to the @ref ObjColMgr to
/// check for collision. Humps become trickable once the geyser's scale is 2 or greater.
bool ObjectFlamePoleFoot::checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);
    f32 scale = getScaleY(timeOffset);

    if (!m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut)) {
        return false;
    }

    if (2.0f < scale) {
        CollisionDirector::Instance()->setCurrentCollisionTrickable(true);
    }

    return true;
}

/// @addr{0x8067F6B8}
/// @brief Calculates the current state based off framecount within the cycle duration
/// @details If the current frame corresponds to a different state than the current one, updates
/// @ref m_nextStateId so the geyser can transition to that state.
void ObjectFlamePoleFoot::calcStates() {
    u32 frame = static_cast<s32>(System::RaceManager::Instance()->timer() - m_initDelay);
    m_cycleFrame = frame % (m_extraCycleFrames + CYCLE_DURATION);

    // Access the array of state timers to see which state corresponds with the current frame.
    auto it = std::ranges::upper_bound(STATE_STARTS.begin(), STATE_STARTS.end(), m_cycleFrame);
    auto idx = it - STATE_STARTS.begin() - 1;

    if (idx < 0) {
        return;
    }

    u16 stateId = static_cast<u16>(idx);

    if (m_currentStateId != stateId) {
        m_nextStateId = stateId;
    }
}

/// @brief Helper function to avoid repeated calculation of constant eruption kinematics
/// @details @ref m_initEruptVel and @ref m_eruptAccel are normally re-computed everytime @ref
/// enterEruptingUp() is called. Since the values used to derive these members never change after
/// object initialization, we instead call this helper function once in @ref init() to avoid
/// repeated computation.
void ObjectFlamePoleFoot::initEruptKinematics() {
    f32 t1 = -1.0f;
    f32 t2 = -1.0f;
    f32 vel = m_maxHeight;
    EGG::Mathf::FindRootsQuadratic(static_cast<f32>(ERUPT_UP_DURATION * ERUPT_UP_DURATION),
            -4.0f * m_maxHeight * static_cast<f32>(ERUPT_UP_DURATION),
            4.0f * m_maxHeight * m_maxHeight, t1, t2);

    if (t1 <= 0.0f) {
        t1 = -1.0f;
    }

    if (t2 <= 0.0f) {
        vel = t1;
    }

    m_initEruptVel = vel;
    m_eruptAccel = vel * vel / (2.0f * m_maxHeight);
}

u32 ObjectFlamePoleFoot::s_flamePoleCount = 0;

} // namespace Kinoko::Field
