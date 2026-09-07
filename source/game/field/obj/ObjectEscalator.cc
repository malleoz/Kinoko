#include "ObjectEscalator.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x807FFB20}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @param reverse Whether the escalator should move in reverse
ObjectEscalator::ObjectEscalator(const System::MapdataGeoObj &params, bool reverse /* = false */)
    : ObjectKCL(params), m_initialPos(pos()),
      m_stillFrames(
              {static_cast<s32>(params.setting(2)) * 60, static_cast<s32>(params.setting(4)) * 60}),
      m_speed({(reverse ? -1.0f : 1.0f) *
                      (0.15f * static_cast<f32>(static_cast<s16>(params.setting(1))) / 100.0f),
              (reverse ? -1.0f : 1.0f) *
                      (0.15f * static_cast<f32>(static_cast<s16>(params.setting(3))) / 100.0f),
              (reverse ? -1.0f : 1.0f) *
                      (0.15f * static_cast<f32>(static_cast<s16>(params.setting(5))) / 100.0f)}),
      m_checkColYPosMax(m_initialPos.y + MAX_HEIGHT_OFFSET),
      m_checkColYPosMin(m_initialPos.y + MIN_HEIGHT_OFFSET),
      m_stopFrames({static_cast<f32>(m_stillFrames[0]) - REVERSE_FRAMES_F32,
              static_cast<f32>(m_stillFrames[1]) - REVERSE_FRAMES_F32}),
      m_startFrames({static_cast<f32>(m_stillFrames[0]) + STANDSTILL_FRAMES,
              static_cast<f32>(m_stillFrames[1]) + STANDSTILL_FRAMES}),
      m_fullSpeedFrames(
              {REVERSE_FRAMES_F32 + m_startFrames[0], REVERSE_FRAMES_F32 + m_startFrames[1]}),
      m_midDuration(m_stopFrames[1] - m_fullSpeedFrames[0]) {
    constexpr EGG::Vector3f STEP_DIMS = EGG::Vector3f(0.0f, STEP_HEIGHT, -30.0f);

    m_wrappedStepCount = 0.0f;

    EGG::Matrix34f mat;
    mat.makeR(rot());
    m_stepDims = mat.ps_multVector(STEP_DIMS);
}

/// @addr{0x80803D00}
/// @brief Default virtual destructor
ObjectEscalator::~ObjectEscalator() = default;

/// @addr{0x808011CC}
bool ObjectEscalator::checkCollision(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);

    if (m_checkColYPosMin >= pos.y || pos.y >= m_checkColYPosMax) {
        return false;
    }

    if (!m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut)) {
        return false;
    }

    if ((*maskOut & KCL_TYPE_BIT(COL_TYPE_MOVING_ROAD)) == 0) {
        return false;
    }

    auto *colDir = CollisionDirector::Instance();
    if (!colDir->findClosestCollisionEntry(maskOut, KCL_TYPE_BIT(COL_TYPE_MOVING_ROAD))) {
        return false;
    }

    auto *entry = colDir->closestCollisionEntry();
    if (entry->dist > info->movingFloorDist) {
        info->movingFloorDist = entry->dist;

        u32 t = System::RaceManager::Instance()->timer() - timeOffset;
        info->roadVelocity = m_stepDims * calcSpeed(t);
    }

    return true;
}

/// @addr{0x808014AC}
bool ObjectEscalator::checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);

    if (m_checkColYPosMin >= pos.y || pos.y >= m_checkColYPosMax) {
        return false;
    }

    if (!m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut)) {
        return false;
    }

    if ((*maskOut & KCL_TYPE_BIT(COL_TYPE_MOVING_ROAD)) == 0) {
        return false;
    }

    auto *colDir = CollisionDirector::Instance();
    if (!colDir->findClosestCollisionEntry(maskOut, KCL_TYPE_BIT(COL_TYPE_MOVING_ROAD))) {
        return false;
    }

    auto *entry = colDir->closestCollisionEntry();
    if (entry->dist > info->movingFloorDist) {
        info->movingFloorDist = entry->dist;

        u32 t = System::RaceManager::Instance()->timer() - timeOffset;
        info->roadVelocity = m_stepDims * calcSpeed(t);
    }

    return true;
}

/// @brief Helper function which re-uses some shared code amongst various collision check variants
/// @tparam T CollisionInfo or CollisionInfoPartial
/// @param checkFunc The function to use for the actual collision check
/// @param pos The point to check collision against
/// @param prevPos The previous position of the kart (used to compute collision depth)
/// @param mask The KCL masks to filter collision checks to (ignores other collision types)
/// @param info Out param that collision info is saved to (if any)
/// @param maskOut Type mask of the KCL the kart is colliding with (if any)
/// @return Whether or not a collision occurred
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectEscalator::checkPointImpl(CheckPointFunc<T> checkFunc, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut) {
    if (m_checkColYPosMin > pos.y || pos.y >= m_checkColYPosMax) {
        return false;
    }

    return (m_objColMgr->*checkFunc)(pos, prevPos, mask, info, maskOut);
}

/// @brief Helper function which re-uses some shared code amongst various collision check variants
/// @tparam T CollisionInfo or CollisionInfoPartial
/// @param checkFunc The function to use for the actual collision check
/// @param radius The radius of the sphere to check collision against
/// @param pos The position of the sphere to check collision against
/// @param prevPos The previous position of the kart (used to compute collision depth)
/// @param mask The KCL masks to filter collision checks to (ignores other collision types)
/// @param info Out param that collision info is saved to (if any)
/// @param maskOut Type mask of the KCL the kart is colliding with (if any)
/// @return Whether or not a collision occurred
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectEscalator::checkSphereImpl(CheckSphereFunc<T> checkFunc, f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    if (m_checkColYPosMin > pos.y || pos.y >= m_checkColYPosMax) {
        return false;
    }

    calcScale(timeOffset);
    update(timeOffset);

    return (m_objColMgr->*checkFunc)(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80800ABC}
/// @brief Evaluates a piecewise function to calculate a wrapped step count.
/// @details The escalator's geometry repeats every 20 steps / 200 units, so this function computes
/// a displacement modulo 200. This is the analytical integral of the piecewise-linear speed profile
/// from
/// @ref calcSpeed, height-scaled and wrapped to the span of a single step.
/// @par Piecewise Position Function
/// Let \f$v_0, v_1, v_2\f$ be @ref m_speed, let \f$t_{s0}, t_{s1}\f$ be @ref m_stopFrames, let
/// \f$t_{a0}, t_{a1}\f$ be @ref m_startFrames, let \f$t_{f0} = t_{a0} + R\f$ and
/// \f$t_{f1} = t_{a1} + R\f$ be @ref m_fullSpeedFrames, and let \f$R\f$ be REVERSE_FRAMES_F32
/// (240). Define the cumulative distances at each breakpoint:
/// \f[
/// C_1 = v_0 t_{s0}, \quad C_2 = C_1 + \frac{v_0 R}{2}, \quad C_3 = C_2 + \frac{v_1 R}{2}, \quad
/// C_4 = C_3 + v_1 (t_{s1} - t_{f0}), \quad C_5 = C_4 + \frac{v_1 R}{2}, \quad
/// C_6 = C_5 + \frac{v_2 R}{2} \, .
/// \f]
/// The escalator's total displacement \f$D(t)\f$ is then
/// \f[
/// D(t) =
/// \begin{cases}
///     v_0 t & 0 \le t < t_{s0}
///     \\ C_1 + \frac{v_0}{2}(t - t_{s0})\left(2 - \frac{t - t_{s0}}{R}\right)
///         & t_{s0} \le t < t_{s0} + R
///     \\ C_2 & t_{s0} + R \le t < t_{a0}
///     \\ C_2 + \frac{v_1 (t - t_{a0})^2}{2R} & t_{a0} \le t < t_{f0}
///     \\ C_3 + v_1 (t - t_{f0}) & t_{f0} \le t < t_{s1}
///     \\ C_4 + \frac{v_1}{2}(t - t_{s1})\left(2 - \frac{t - t_{s1}}{R}\right)
///         & t_{s1} \le t < t_{s1} + R
///     \\ C_5 & t_{s1} + R \le t < t_{a1}
///     \\ C_5 + \frac{v_2 (t - t_{a1})^2}{2R} & t_{a1} \le t < t_{f1}
///     \\ C_6 + v_2 (t - t_{f1}) & t \ge t_{f1}
/// \end{cases}
/// \f]
/// Finally, the result is wrapped to the span of a single step, where \f$H\f$ is
/// STEP_HEIGHT and \f$M = 200\f$ is the number of discrete step offsets:
/// \f[
/// \text{calcWrappedStepCount}(t) = \frac{\left(\left\lfloor H \cdot D(t) \right\rfloor \bmod M + M
/// \right) \bmod M}{H} \, .
/// \f]
f32 ObjectEscalator::calcWrappedStepCount(s32 t) {
    constexpr s32 REVERSE_FRAMES_S32 = static_cast<s32>(REVERSE_FRAMES_F32);
    constexpr s32 DISCRETE_STEP_OFFSETS = 200;

    // Time since escalator began stopping
    std::array<s32, 2> dtStop = {static_cast<s32>(static_cast<f32>(t) - m_stopFrames[0]),
            static_cast<s32>(static_cast<f32>(t) - m_stopFrames[1])};

    // Time since escalator started moving (second and third time)
    std::array<s32, 2> dtStart = {static_cast<s32>(static_cast<f32>(t) - m_startFrames[0]),
            static_cast<s32>(static_cast<f32>(t) - m_startFrames[1])};

    // Time since escalator started moving at full speed
    std::array<s32, 2> dtFullSpeed = {static_cast<s32>(static_cast<f32>(t) - m_fullSpeedFrames[0]),
            static_cast<s32>(static_cast<f32>(t) - m_fullSpeedFrames[1])};

    // Each dist lambda represents the displacement from each sub-function of the piecewise function

    // Escalator has not stopped yet
    auto dist0 = [this](s32 t) -> f64 {
        t = std::clamp<s32>(t, 0, static_cast<s32>(m_stopFrames[0]));
        return static_cast<f64>(static_cast<f32>(t) * m_speed[0]);
    };

    // Escalator is slowing down for the first time
    auto dist1 = [=, this]() -> f64 {
        f64 speed = static_cast<f64>(m_speed[0]);
        f64 t = static_cast<f64>(std::clamp<s32>(dtStop[0], 0, REVERSE_FRAMES_S32));
        return 0.5 * t * (speed + speed * (1.0 - t / static_cast<f64>(REVERSE_FRAMES_F32)));
    };

    // Escalator is speeding up after switching directions once
    auto dist2 = [=, this]() -> f64 {
        f64 t = static_cast<f64>(std::clamp<s32>(dtStart[0], 0, REVERSE_FRAMES_S32));
        return t * (0.5 * t * static_cast<f64>(m_speed[1])) / static_cast<f64>(REVERSE_FRAMES_F32);
    };

    // Escalator is moving full speed after switching directions once
    auto dist3 = [=, this](s32 t) -> f64 {
        t = std::clamp<s32>(dtFullSpeed[0], 0, m_midDuration);
        return static_cast<f64>(static_cast<f32>(t) * m_speed[1]);
    };

    // Escalator is slowing down for the second time
    auto dist4 = [=, this]() -> f64 {
        f64 speed = static_cast<f64>(m_speed[1]);
        f64 t = static_cast<f32>(std::clamp<s32>(dtStop[1], 0, REVERSE_FRAMES_S32));
        return 0.5 * t * (speed + speed * (1.0 - t / static_cast<f64>(REVERSE_FRAMES_F32)));
    };

    // Escalator is speeding up after switching directions twice
    auto dist5 = [=, this]() -> f64 {
        f64 speed = static_cast<f64>(m_speed[2]);
        f64 t = static_cast<f64>(std::clamp<s32>(dtStart[1], 0, REVERSE_FRAMES_S32));
        return (t * (0.5 * t * speed) / static_cast<f64>(REVERSE_FRAMES_F32));
    };

    // Escalator is moving full speed after switching directions twice
    auto dist6 = [=, this]() -> f64 {
        f64 t = static_cast<f64>(std::max(dtFullSpeed[1], 0));
        return t * static_cast<f64>(m_speed[2]);
    };

    f64 totalDist = dist6() + dist5() + dist4() + dist3(t) + dist2() + dist0(t) + dist1();
    s32 result = static_cast<s32>(STEP_HEIGHT * static_cast<f32>(totalDist));
    f32 fin = static_cast<f32>(result % DISCRETE_STEP_OFFSETS);

    if (fin < 0.0f) {
        fin += static_cast<f32>(DISCRETE_STEP_OFFSETS);
    }

    return fin / STEP_HEIGHT;
}

/// @addr{0x80800FBC}
/// @brief Calculates the speed of the escalator at a given time t
/// @details This is the derivative of the position function @ref calcWrappedStepCount() integrates.
/// @par Piecewise Speed Function
/// Using the same symbols as @ref calcWrappedStepCount(): \f$v_0, v_1, v_2\f$ are @ref m_speed,
/// \f$t_{s0}, t_{s1}\f$ are @ref m_stopFrames, \f$t_{a0}, t_{a1}\f$ are @ref m_startFrames,
/// \f$t_{f0} = t_{a0} + R\f$ and \f$t_{f1} = t_{a1} + R\f$ are @ref m_fullSpeedFrames, and
/// \f$R\f$ is REVERSE_FRAMES_F32.
/// \f[
/// v(t) =
/// \begin{cases}
///     v_0 & t < t_{s0}
///     \\ v_0 \frac{t_{s0} + R - t}{R} & t_{s0} \le t < t_{s0} + R
///     \\ 0 & t_{s0} + R \le t \le t_{a0}
///     \\ v_1 \frac{t - t_{a0}}{R} & t_{a0} < t < t_{f0}
///     \\ v_1 & t_{f0} \le t < t_{s1}
///     \\ v_1 \frac{t_{s1} + R - t}{R} & t_{s1} \le t < t_{s1} + R
///     \\ 0 & t_{s1} + R \le t \le t_{a1}
///     \\ v_2 \frac{t - t_{a1}}{R} & t_{a1} < t < t_{f1}
///     \\ v_2 & t \ge t_{f1}
/// \end{cases}
/// \f]
f32 ObjectEscalator::calcSpeed(s32 t) {
    // Escalator has not stopped yet
    if (static_cast<f32>(t) < m_stopFrames[0]) {
        return m_speed[0];
    }

    // Escalator is slowing down for the first time
    if (t < m_stillFrames[0]) {
        return m_speed[0] * static_cast<f32>(m_stillFrames[0] - t) / REVERSE_FRAMES_F32;
    }

    // Standstill right after stopping for the first time
    if (static_cast<f32>(t) <= m_startFrames[0]) {
        return 0.0f;
    }

    // Escalator is speeding up after switching directions once
    if (static_cast<f32>(t) < REVERSE_FRAMES_F32 + m_startFrames[0]) {
        return m_speed[1] * (static_cast<f32>(t) - m_startFrames[0]) / REVERSE_FRAMES_F32;
    }

    // Escalator is moving full speed after switching directions once
    if (static_cast<f32>(t) < m_stopFrames[1]) {
        return m_speed[1];
    }

    // Escalator is slowing down for the second time
    if (t < m_stillFrames[1]) {
        return m_speed[1] * static_cast<f32>(m_stillFrames[1] - t) / REVERSE_FRAMES_F32;
    }

    // Standstill right after stopping for the second/final time
    if (static_cast<f32>(t) <= m_startFrames[1]) {
        return 0.0f;
    }

    // Escalator is speeding up after switching directions twice
    if (static_cast<f32>(t) < REVERSE_FRAMES_F32 + m_startFrames[1]) {
        return m_speed[2] * (static_cast<f32>(t) - m_startFrames[1]) / REVERSE_FRAMES_F32;
    }

    // Escalator is moving full speed after switching directions twice
    return m_speed[2];
}

} // namespace Kinoko::Field
