#include "ObjectFlamePoleFoot.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

#include <algorithm>

namespace Kinoko::Field {

/// @addr{0x8067E6F4}
ObjectFlamePoleFoot::ObjectFlamePoleFoot(const System::MapdataGeoObj &params)
    : ObjectKCL(params), StateManager(this, STATE_ENTRIES), m_extraCycleFrames(params.setting(0)),
      m_initDelay(params.setting(1)) {
    m_maxScale = static_cast<f32>(params.setting(2));

    ++s_flamePoleCount;

    if (m_maxScale == 0.0f) {
        m_maxScale = 3.0f + static_cast<f32>(s_flamePoleCount % 3);
    }

    m_pole = EGG::egg_new<ObjectFlamePole>(params, pos(), rot(), scale());
    m_pole->load();
}

/// @addr{0x8067EBE0}
ObjectFlamePoleFoot::~ObjectFlamePoleFoot() {
    s_flamePoleCount = 0;
}

/// @addr{0x8067EC94}
void ObjectFlamePoleFoot::init() {
    constexpr f32 NORMALIZATION = static_cast<f32>(CYCLE_DURATION) * (7.0f - 1.0f);

    m_nextStateId = 0;
    m_cycleFrame = 0;
    m_eruptUpDuration = static_cast<s32>(0.1f * NORMALIZATION / 7.0f);
    m_eruptDownDuration = static_cast<s32>(0.2f * NORMALIZATION / 7.0f);

    s32 state1 = static_cast<s32>(0.3f * NORMALIZATION / 7.0f);
    s32 state2 = state1 + m_eruptUpDuration;
    s32 state3 = state2 + static_cast<s32>(static_cast<f32>(CYCLE_DURATION) * 1.0f / 7.0f);
    s32 state4 = state3 + m_eruptDownDuration;
    s32 state5 = state4 + static_cast<s32>(0.4f * NORMALIZATION / 7.0f);
    m_stateStart = {{0, state1, state2, state3, state4, state5}};

    m_eruptDownVel = (m_maxScale - 1.0f) / static_cast<f32>(state1);
    m_maxHeight = ObjectFlamePole::HEIGHT * m_maxScale;
    m_scaleDelta = (300.0f + m_maxHeight) / static_cast<f32>(m_eruptDownDuration);
    m_pole->setActive(false);
    m_pole->disableCollision();

    EGG::Vector3f polePos = m_pole->pos();
    polePos.y = pos().y - ObjectFlamePole::HEIGHT * m_maxScale;
    m_pole->setPos(polePos);
}

/// @addr{0x8067EF70}
void ObjectFlamePoleFoot::calc() {
    if (System::RaceManager::Instance()->timer() < m_initDelay) {
        return;
    }

    calcStates();

    StateManager::calc();

    f32 scale = getScaleY(0);
    setScale(scale);

    EGG::Vector3f polePos = m_pole->pos();
    m_pole->setPos(EGG::Vector3f(polePos.x, m_heightOffset + (pos().y - m_maxHeight), polePos.z));
    m_pole->setScale(m_maxScale);
}

/** @addr{0x8067F6B8}
 * @brief Calculates the current state based off framecount within the cycle duration
 * @details References the values in m_stateState as follows:
 * Let \f$t\f$ be @ref m_cycleFrame and \f$s_i\f$ be @ref m_stateStart "m_stateStart[i]". Then
 * \f[
 * \text{stateId}(t) =
 * \begin{cases}
 *     0 & 0 \le t < s_1 \\
 *     1 & s_1 \le t < s_2 \\
 *     2 & s_2 \le t < s_3 \\
 *     3 & s_3 \le t < s_4 \\
 *     4 & s_4 \le t < s_5 \\
 *     5 & t \ge s_5
 * \end{cases}
 * \f]
 * where
 * \f[
 * \begin{aligned}
 * N &= 6 \cdot \text{CYCLE_FRAMES} \\
 * s_1 &= \left\lfloor \frac{0.3 N}{7} \right\rfloor \\
 * s_2 &= s_1 + \left\lfloor \frac{0.1 N}{7} \right\rfloor \\
 * s_3 &= s_2 + \left\lfloor \frac{\text{CYCLE_FRAMES}}{7} \right\rfloor \\
 * s_4 &= s_3 + \left\lfloor \frac{0.2 N}{7} \right\rfloor \\
 * s_5 &= s_4 + \left\lfloor \frac{0.4 N}{7} \right\rfloor
 * \end{aligned}
 * \f]
 * Simplifying further, we have:
 * \f[
 * \text{stateId}(t) =
 * \begin{cases}
 *     0 & 0 \le t < 138 \\
 *     1 & 138 \le t < 184 \\
 *     2 & 184 \le t < 261 \\
 *     3 & 261 \le t < 353 \\
 *     4 & 353 \le t < 538 \\
 *     5 & t \ge 538
 * \end{cases}
 * \f]
 */
void ObjectFlamePoleFoot::calcStates() {
    u32 frame = static_cast<s32>(System::RaceManager::Instance()->timer() - m_initDelay);
    m_cycleFrame = frame % (m_extraCycleFrames + CYCLE_DURATION);

    // Access the array of state timers to see which state corresponds with the current frame.
    auto it = std::ranges::upper_bound(m_stateStart.begin(), m_stateStart.end(), m_cycleFrame);
    auto idx = it - m_stateStart.begin() - 1;

    if (idx < 0) {
        return;
    }

    u16 stateId = static_cast<u16>(idx);

    if (m_currentStateId != stateId) {
        m_nextStateId = stateId;
    }
}

/// @brief Updates the scale of the geyser hump based off framecount in the cycle
/// @todo THIS IS MISSING AN ADDRESS
f32 ObjectFlamePoleFoot::getScaleY(u32 timeOffset) const {
    u32 frame = System::RaceManager::Instance()->timer() - timeOffset;
    if (frame < m_initDelay) {
        return 1.0f;
    }

    s32 cycleFrame = frame - m_initDelay;
    cycleFrame %= m_extraCycleFrames + CYCLE_DURATION;

    if (cycleFrame >= m_stateStart[5]) {
        return 1.0f;
    }

    if (cycleFrame >= m_stateStart[4]) {
        return std::max(1.0f,
                m_maxScale - m_eruptDownVel * static_cast<f32>(m_eruptDownDuration / 2) -
                        m_eruptDownVel * static_cast<f32>(cycleFrame - m_stateStart[4]));
    }

    if (cycleFrame >= m_stateStart[3]) {
        s32 framesSince194 = cycleFrame - m_stateStart[3];
        s32 half17c = m_eruptDownDuration / 2;
        if (framesSince194 > half17c) {
            return m_maxScale - m_eruptDownVel * static_cast<f32>(framesSince194 - half17c);
        } else {
            return m_maxScale;
        }
    }

    if (cycleFrame >= m_stateStart[2] || cycleFrame >= m_stateStart[1]) {
        return m_maxScale;
    }

    if (cycleFrame >= 0) {
        return std::min(m_maxScale, m_eruptDownVel * static_cast<f32>(cycleFrame) + 1.0f);
    }

    return 1.0f;
}

/// @addr{0x8067FE88}
/// @copydoc ObjectKCL::checkCollision()
/// @details Humps become trickable once the scale is 2 or greater
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
/// @details Humps become trickable once the scale is 2 or greater
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

/// @addr{0x8067F2F4}
/// @brief Runs once when the geyser begins raising the flame pole
void ObjectFlamePoleFoot::enterEruptingUp() {
    m_pole->setActive(true);
    m_pole->enableCollision();
    m_heightOffset = 0.0f;

    f32 t1 = -1.0f;
    f32 t2 = -1.0f;
    f32 vel = m_maxHeight;
    EGG::Mathf::FindRootsQuadratic(static_cast<f32>(m_eruptUpDuration * m_eruptUpDuration),
            -4.0f * m_maxHeight * static_cast<f32>(m_eruptUpDuration),
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

/// @addr{0x8067F484}
/// @brief Runs every frame when the flame pole is raising up
void ObjectFlamePoleFoot::calcEruptingUp() {
    f32 frame = static_cast<f32>(m_cycleFrame - m_stateStart[1]);
    m_heightOffset =
            std::min(m_maxHeight, m_initEruptVel * frame - frame * 0.5f * m_eruptAccel * frame);
}

/// @addr{0x8067F544}
/// @brief Runs every frame after the flame pole reaches max height and before falling to dormancy
void ObjectFlamePoleFoot::calcEruptingStay() {
    constexpr f32 AMPLITUDE = 50.0f;

    f32 angle = 360.0f * static_cast<f32>(m_cycleFrame - m_stateStart[2]) / 30.0f;
    m_heightOffset = m_eruptedHeightOffset + AMPLITUDE * EGG::Mathf::SinFIdx(DEG2FIDX * angle);
}

u32 ObjectFlamePoleFoot::s_flamePoleCount = 0;

} // namespace Kinoko::Field
