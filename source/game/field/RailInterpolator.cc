#include "RailInterpolator.hh"

#include "game/field/RailManager.hh"

namespace Kinoko::Field {

/// @addr{0x806ED160}
/// @brief Constructor that initializes the rail interpolator with the provided speed and rail index
/// @param speed The default speed of the interpolator
/// @param idx The index of the rail to interpolate along
RailInterpolator::RailInterpolator(f32 speed, u32 idx)
    : m_railIdx(idx), m_points(RailManager::Instance()->rail(idx)->points()), m_speed(speed),
      m_currSpeed(0.0f) {
    auto *rail = RailManager::Instance()->rail(idx);
    m_pointCount = rail->pointCount();
    m_isOscillating = rail->isOscillating();
}

/// @addr{0x806ED53C}
/// @brief Default virtual destructor
RailInterpolator::~RailInterpolator() = default;

/// @addr{0806ED24C}
/// @brief Returns the up vector of the point at the given index
const EGG::Vector3f &RailInterpolator::floorNrm(size_t idx) const {
    return RailManager::Instance()->rail(m_railIdx)->floorNrm(idx);
}

/// @addr{0x806ED30C}
/// @brief Returns the sum of the lengths of the edges along the rail
f32 RailInterpolator::railLength() const {
    return RailManager::Instance()->rail(m_railIdx)->getPathLength();
}

/// @addr{0x806ED34C}
/// @brief Caches the current and next point velocities
/// @details Falls back to the default rail speed if either is 0
/// @bug This is callable with an invalid m_nextPointIdx, so we safeguard the invalid span lookup by
/// just setting the next point vel to 0. This invalid state is resolved very shortly after, but the
/// vel propagates to the next frame. If t is ever not 0, this leads to undefined behavior, so we
/// guard against it here.
void RailInterpolator::calcVelocities() {
    m_currPointVel = static_cast<f32>(m_points[m_currPointIdx].setting[0]);
    m_nextPointVel =
            shouldChangeDirection() ? 0.0f : static_cast<f32>(m_points[m_nextPointIdx].setting[0]);

    if (m_currPointVel == 0.0f) {
        m_currPointVel = m_speed;
    }

    if (m_nextPointVel == 0.0f) {
        m_nextPointVel = m_speed;
    }

    // In the invalid state, if t is ever not 0, this leads to undefined behavior.
    // So we guard against it here.
    if (shouldChangeDirection()) {
        ASSERT(m_segmentT == 0.0f);
    }
}

/// @addr{0x806F0880}
/// @brief Updates the next point index to reflect the direction change and toggles the forward flag
void RailInterpolator::calcDirectionChange() {
    if (!m_isOscillating) {
        return;
    }

    if (m_forward) {
        m_nextPointIdx -= 2;
    } else {
        m_nextPointIdx += 2;
    }

    m_forward = !m_forward;
}

/// @brief Updates the current and next point indices based on the current direction of movement
void RailInterpolator::calcNextIndices() {
    if (m_forward) {
        ++m_currPointIdx;
        ++m_nextPointIdx;
    } else {
        --m_currPointIdx;
        --m_nextPointIdx;
    }

    if (!m_isOscillating) {
        if (m_nextPointIdx == m_pointCount) {
            m_nextPointIdx = 0;
        }

        if (m_currPointIdx == m_pointCount) {
            m_currPointIdx = 0;
        }
    }
}

/// @addr{0x806EFDC4}
/// @brief Constructor that initializes the linear rail interpolator with the provided speed and
/// rail index
/// @param speed The default speed of the interpolator
/// @param idx The index of the rail to interpolate along
RailLinearInterpolator::RailLinearInterpolator(f32 speed, u32 idx) : RailInterpolator(speed, idx) {
    m_transitions = RailManager::Instance()->rail(m_railIdx)->getLinearTransitions();
    init(0.0f, 0);
}

/// @addr{0x806F094C}
/// @brief Default virtual destructor
RailLinearInterpolator::~RailLinearInterpolator() = default;

/// @addr{0x806EFEAC}
void RailLinearInterpolator::init(f32 t, u32 idx) {
    m_segmentT = t;
    m_currPointIdx = idx;

    bool isLastPoint = (static_cast<u16>(idx) == m_pointCount - 1);
    m_nextPointIdx = isLastPoint ? idx - 1 : idx + 1;
    m_forward = isLastPoint ? !m_isOscillating : true;

    m_curPos = m_points[m_currPointIdx].pos;
    m_currVel = m_points[m_nextPointIdx].pos - m_curPos;
    m_curTangentDir = m_currVel;
    m_curTangentDir.normalise2();
    m_currSpeed = m_speed;
    m_currPointVel = m_speed;
    m_nextPointVel = m_speed;
    m_usePerPointVelocities = false;
    m_currSegmentVel = m_speed / m_currVel.length();
}

/// @addr{0x806F0050}
RailInterpolator::Status RailLinearInterpolator::calc() {
    if (m_usePerPointVelocities) {
        updateVel();
    }

    m_segmentT += m_currSegmentVel;
    m_curPos = lerp(m_segmentT, m_currPointIdx, m_nextPointIdx);

    if (m_segmentT <= 1.0f) {
        return Status::InProgress;
    }

    Status status = Status::SegmentEnd;

    calcNextSegment();

    if (shouldChangeDirection()) {
        status = Status::ChangingDirection;

        calcDirectionChange();
    }

    m_currVel = m_points[m_nextPointIdx].pos - m_points[m_currPointIdx].pos;
    m_curTangentDir = m_currVel;
    m_currSegmentVel = m_currSpeed / m_currVel.length();
    m_curTangentDir.normalise2();

    return status;
}

/// @addr{0x806F02EC}
/// @details Uses linear interpolation to get the position and tangent direction along the rail at a
/// specific t distance behind the current position.
void RailLinearInterpolator::evalPositionAndTangentBehind(f32 t, EGG::Vector3f &currPos,
        EGG::Vector3f &curTangentDir) const {
    s16 currIdx = 0;
    f32 len = 0.0f;

    getPathLocation(t, currIdx, len);

    s16 nextIdx = currIdx + 1;
    if (nextIdx == m_pointCount) {
        nextIdx = 0;
    }

    currPos = lerp(len, currIdx, nextIdx);
    curTangentDir = m_transitions[currIdx].m_dir;
}

/// @addr{0x806F041C}
void RailLinearInterpolator::getPathLocation(f32 t, s16 &idx, f32 &len) const {
    if (!m_forward) {
        return;
    }

    f32 dist = m_segmentT * m_transitions[m_currPointIdx].m_length;
    if (t <= dist) {
        idx = m_currPointIdx;
        len = (dist - t) * m_transitions[m_currPointIdx].m_lengthInv;
        return;
    }

    for (s32 i = 0; i < m_pointCount; ++i) {
        s32 currIdx = m_currPointIdx - 1;
        if (currIdx == -1) {
            currIdx = m_pointCount - 1;
        }

        currIdx -= i;
        if (currIdx < 0) {
            currIdx += m_pointCount;
        }

        dist += m_transitions[currIdx].m_length;
        if (t <= dist) {
            idx = currIdx;
            len = (dist - t) * m_transitions[currIdx].m_lengthInv;
            return;
        }
    }
}

/// @addr{0x806F0610}
/// @brief Runs when the interpolator reaches the end of a segment
/// @details Updates the current and next point indices, recalculates the current velocity, and
/// updates the segmentT value to reflect the new segment.
/// @bug The game can access POTI points out-of-bounds, but then course-corrects by
/// setting m_segmentT to 0.0f once it detects an invalid nextPointIdx, but after
/// it already read from undefined memory. To address this, we pre-emptively set m_segmentT to 0.0f
/// and later catch the invalid state in @ref calcVelocities() to avoid undefined behavior.
void RailLinearInterpolator::calcNextSegment() {
    calcNextIndices();

    if (shouldChangeDirection()) {
        m_segmentT = 0.0f;
    } else {
        f32 prevDirLength = m_currVel.length();
        m_currVel = m_points[m_nextPointIdx].pos - m_points[m_currPointIdx].pos;
        m_segmentT = ((m_segmentT - 1.0f) * prevDirLength) / m_currVel.length();

        if (m_segmentT > 1.0f) {
            m_segmentT = 0.99f;
        }
    }

    if (m_usePerPointVelocities) {
        calcVelocities();
    }
}

/// @addr{0x806EE830}
/// @brief Constructor that initializes the smooth rail interpolator with the provided speed and
/// rail index
/// @param speed The default speed of the interpolator
/// @param idx The index of the rail to interpolate along
RailSmoothInterpolator::RailSmoothInterpolator(f32 speed, u32 idx) : RailInterpolator(speed, idx) {
    auto *rail = RailManager::Instance()->rail(m_railIdx);
    m_transitions = rail->getSplineTransitions();
    m_estimatorSampleCount = static_cast<u32>(rail->getEstimatorSampleCount());
    m_estimatorStep = rail->getEstimatorStep();
    m_pathPercentages = rail->getPathPercentages();

    init(0.0f, 0);
}

/// @addr{0x806EF944}
/// @brief Default virtual destructor
RailSmoothInterpolator::~RailSmoothInterpolator() = default;

/// @addr{0x806EE924}
void RailSmoothInterpolator::init(f32 t, u32 idx) {
    m_segmentT = t;

    m_currPointIdx = idx;

    if (idx == static_cast<u32>(m_pointCount - 1) && m_isOscillating) {
        m_nextPointIdx = m_currPointIdx - 1;
        m_forward = false;
        m_curTangentDir = calcCubicBezierTangentDir(m_segmentT, m_transitions[m_currPointIdx]);
        m_currSegmentVel = m_currSpeed * m_transitions[m_nextPointIdx].m_lengthInv;
    } else {
        m_nextPointIdx = idx + 1 == m_pointCount ? 0 : idx + 1;
        m_forward = true;
        m_curTangentDir = calcCubicBezierTangentDir(m_segmentT, m_transitions[m_currPointIdx]);
        m_currSegmentVel = m_currSpeed * m_transitions[m_currPointIdx].m_lengthInv;
    }

    m_curPos = m_points[m_currPointIdx].pos;
    m_prevPos = m_points[m_currPointIdx].pos;
    m_currSpeed = m_speed;
    m_currPointVel = m_speed;
    m_nextPointVel = m_speed;
    m_velocity = 0.0f;
    m_usePerPointVelocities = false;
}

/// @addr{0x806EEBEC}
RailInterpolator::Status RailSmoothInterpolator::calc() {
    if (m_usePerPointVelocities) {
        updateVel();
    }

    m_prevPos = m_curPos;

    f32 t = m_forward ? calcT(m_segmentT) : calcT(1.0f - m_segmentT);

    calcCubicBezier(t, m_currPointIdx, m_nextPointIdx, m_curPos, m_curTangentDir);

    EGG::Vector3f deltaPos = m_curPos - m_prevPos;
    m_velocity = deltaPos.length();
    m_segmentT += m_currSegmentVel;

    if (m_segmentT <= 1.0f) {
        return Status::InProgress;
    }

    Status status = Status::SegmentEnd;

    calcNextSegment();

    if (shouldChangeDirection()) {
        status = Status::ChangingDirection;

        calcDirectionChange();
    }

    if (m_forward) {
        m_currSegmentVel = m_currSpeed * m_transitions[m_currPointIdx].m_lengthInv;
    } else {
        m_currSegmentVel = m_currSpeed * m_transitions[m_nextPointIdx].m_lengthInv;
    }

    return status;
}

/// @addr{0x806EEEBC}
/// @details Evaluates the cubic bezier curve at the given t value to get the position and tangent
/// direction at aspecific t distance behind the current position.
void RailSmoothInterpolator::evalPositionAndTangentBehind(f32 t, EGG::Vector3f &currPos,
        EGG::Vector3f &curTangentDir) const {
    s16 currIdx = 0;
    f32 len = 0.0f;

    getPathLocation(t, currIdx, len);

    s16 nextIdx = currIdx + 1;
    if (nextIdx == m_pointCount) {
        nextIdx = 0;
    }

    len = m_forward ? calcT(len) : calcT(1.0f - len);

    calcCubicBezier(len, currIdx, nextIdx, currPos, curTangentDir);
}

/// @addr{0x806EEFA0}
void RailSmoothInterpolator::getPathLocation(f32 t, s16 &idx, f32 &len) const {
    if (!m_forward) {
        return;
    }

    f32 dist = m_segmentT * m_transitions[m_currPointIdx].m_length;
    if (t <= dist) {
        idx = m_currPointIdx;
        len = (dist - t) * m_transitions[m_currPointIdx].m_lengthInv;
        return;
    }

    for (s32 i = 0; i < m_pointCount; ++i) {
        s32 currIdx = m_currPointIdx - 1;
        if (currIdx == -1) {
            currIdx = m_pointCount - 1;
        }

        currIdx -= i;
        if (currIdx < 0) {
            currIdx += m_pointCount;
        }

        dist += m_transitions[currIdx].m_length;
        if (t <= dist) {
            idx = currIdx;
            len = (dist - t) * m_transitions[currIdx].m_lengthInv;
            return;
        }
    }
}

/// @addr{0x806EF454}
/// @brief Evaluates the tangent direction of a cubic bezier curve at the given parameter t
/// @param t The parameter along the curve to evaluate, in the range [0, 1]
/// @param trans The bezier curve to evaluate
EGG::Vector3f RailSmoothInterpolator::calcCubicBezierTangentDir(f32 t,
        const RailSplineTransition &trans) const {
    EGG::Vector3f c1 = trans.m_p0 * -1.0f + trans.m_p1 * 3.0f - (trans.m_p2 * 3.0f) + trans.m_p3;
    EGG::Vector3f c2 = trans.m_p0 * 3.0f - trans.m_p1 * 6.0f + trans.m_p2 * 3.0f;
    EGG::Vector3f c3 = trans.m_p0 * -3.0f + trans.m_p1 * 3.0f;
    EGG::Vector3f ret = c1 * 3.0f * (t * t) + c2 * 2.0f * t + c3;

    ret.normalise2();

    if (!m_forward) {
        ret *= -1.0f;
    }

    return ret;
}

/// @addr{0x806EF0F8}
/// @brief Maps a time-based parameter t to a bezier curve parameter
/// @param t The time-based parameter to map, in the range [0, 1]
/// @details m_segmentT is a normalized time-based position along the current segment. A cubic
/// bezier is not parametrized by arc length; if you just evaluate the bezier curve at m_segmentT,
/// the object would speed up and slow down unnaturally as it moves along the curve. To address
/// this, this function linearly interpolates within an array of precomputed arc length percentages
/// across the curve.
f32 RailSmoothInterpolator::calcT(f32 t) const {
    u16 sampleIdx = m_forward ? m_currPointIdx * m_estimatorSampleCount :
                                m_nextPointIdx * m_estimatorSampleCount;

    f32 delta = 0.0f;
    u16 idx = 0;

    for (u16 i = 0; i < m_estimatorSampleCount - 1; ++i) {
        f32 currPercent = m_pathPercentages[sampleIdx + i];
        f32 nextPercent = m_pathPercentages[sampleIdx + i + 1];

        if (currPercent <= t && nextPercent > t) {
            delta = (t - currPercent) / (nextPercent - currPercent);
            idx = i;
        }
    }

    f32 lastPercent = m_pathPercentages[sampleIdx + m_estimatorSampleCount - 1];

    if (lastPercent <= t && 1.0f >= t) {
        idx = m_estimatorSampleCount - 1;
        delta = (t - lastPercent) / (1.0f - lastPercent);
    }

    return m_estimatorStep * static_cast<f32>(idx) + m_estimatorStep * delta;
}

/// @addr{0x806EF664}
/// @brief Runs when the interpolator reaches the end of a segment
/// @details Updates the current and next point indices, updates the segmentT value to reflect the
/// new segment, and updates per-point velocities, if applicable.
void RailSmoothInterpolator::calcNextSegment() {
    f32 nextT = m_segmentT - 1.0f;

    if (m_isOscillating) {
        if (m_nextPointIdx == 0 || m_nextPointIdx == m_pointCount - 1) {
            m_segmentT = 0.0f;
        } else if (m_forward) {
            m_segmentT = nextT * m_transitions[m_currPointIdx].m_length *
                    m_transitions[m_nextPointIdx].m_lengthInv;
        } else {
            m_segmentT = nextT * m_transitions[m_currPointIdx - 1].m_length *
                    m_transitions[m_nextPointIdx - 1].m_lengthInv;
        }
    } else {
        m_segmentT = nextT * m_transitions[m_currPointIdx].m_length *
                m_transitions[m_nextPointIdx].m_lengthInv;
    }

    if (m_segmentT > 1.0f) {
        m_segmentT = 0.99f;
    }

    calcNextIndices();

    if (m_usePerPointVelocities) {
        calcVelocities();
    }
}

} // namespace Kinoko::Field
