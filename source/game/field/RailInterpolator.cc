#include "RailInterpolator.hh"

namespace Kinoko::Field {

/// @addr{0x806ED34C}
/// @brief Caches the current and next point velocities
/// @details Falls back to the default rail speed if either is 0
/// @bug This is callable with an invalid m_nextPointIdx, so we safeguard the invalid span lookup by
/// just setting the next point vel to 0. In the base game, this invalid state is resolved very
/// shortly after, but the vel propagates to the next frame. If @ref m_segmentT is ever not 0, this
/// leads to undefined behavior, so we guard against it here.
void RailInterpolator::calcVelocities() {
    m_currPointVel = static_cast<f32>(m_points[m_currPointIdx].setting[0]);
    m_nextPointVel =
            hasReachedEndOfRail() ? 0.0f : static_cast<f32>(m_points[m_nextPointIdx].setting[0]);

    if (m_currPointVel == 0.0f) {
        m_currPointVel = m_baseSpeed;
    }

    if (m_nextPointVel == 0.0f) {
        m_nextPointVel = m_baseSpeed;
    }

    // In the invalid state, if t is ever not 0, this leads to undefined behavior.
    // So we guard against it here.
    if (hasReachedEndOfRail()) {
        ASSERT(m_segmentT == 0.0f);
    }
}

/// @addr{0x806F0880}
/// @brief Updates the next point index to reflect the direction change and toggles the forward flag
/// @details If @ref m_isOscillating is `false`, then this rail does not change direction, so early
/// returns. Otherwise, @ref m_nextPointIdx is incremented or decremented by 2, depending on the
/// direction of movement. Finally, inverts @ref m_forward to reflect the change in direction.
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
/// @details Increments or decrements the indices depending on whether the rail is moving forward or
/// backward. The indices wrap around to zero if the rail is not oscillating and reaches the end.
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

/// @brief Shared implementation of @ref RailInterpolator::getPathLocation(), templated on the
/// transition type so that @ref RailLinearInterpolator and @ref RailSmoothInterpolator can each
/// pass their own span of @ref RailLineTransition or @ref RailSplineTransition
/// @tparam T The type of the rail transition, either @ref RailLineTransition or @ref
/// RailSplineTransition
/// @param transitions The span of rail transitions
/// @param dist The distance behind the current position along the rail
/// @param idx The index of the segment that corresponds to `dist`
/// @param t The normalized position along the segment that corresponds to `dist`
/// @details If the rail interpolator is currently moving backwards, then this function early
/// returns. If the distance `dist` lies within the current segment, then `idx` is set to @ref
/// m_currPointIdx and `t` is set to the normalized distance along the segment. Otherwise, the
/// function iterates through previous segments to find the segment that contains the distance
/// `dist` and calculates the corresponding normalized position along that segment.
template <typename T>
    requires std::is_same_v<T, RailLineTransition> || std::is_same_v<T, RailSplineTransition>
void RailInterpolator::getPathLocationImpl(std::span<const T> transitions, f32 dist, s16 &idx,
        f32 &t) const {
    if (!m_forward) {
        return;
    }

    f32 currDist = m_segmentT * transitions[m_currPointIdx].m_length;
    if (dist <= currDist) {
        idx = m_currPointIdx;
        t = (currDist - dist) * transitions[m_currPointIdx].m_lengthInv;
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

        currDist += transitions[currIdx].m_length;
        if (dist <= currDist) {
            idx = currIdx;
            t = (currDist - dist) * transitions[currIdx].m_lengthInv;
            return;
        }
    }
}

/// @addr{0x806EFEAC}
/// @copydoc RailInterpolator::init()
/// @details Sets @ref m_segmentT and @ref m_currPointIdx to the provided `t` and `idx`
/// respectively. Sets @ref m_nextPointIdx by incrementing or decrementing `m_currPointIdx`
/// depending on whether it is the last point. Also sets the forward flag accordingly. Sets @ref
/// m_curPos and @ref m_currVel based on the current and next points. Sets @ref m_curTangentDir
/// based on the direction of the current velocity. Sets @ref m_currSpeed, @ref m_currPointVel, and
/// @ref m_nextPointVel to @ref m_baseSpeed. Initializes @ref m_usePerPointVelocities to `false` and
/// updates @ref m_currSegmentVel based on @ref m_baseSpeed and the distance between the current and
/// next points.
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
    m_currSpeed = m_baseSpeed;
    m_currPointVel = m_baseSpeed;
    m_nextPointVel = m_baseSpeed;
    m_usePerPointVelocities = false;
    m_currSegmentVel = m_baseSpeed / m_currVel.length();
}

/// @addr{0x806F0050}
/// @copydetails RailInterpolator::calc()
/// @details If @ref m_usePerPointVelocities is `true`, then calls @ref updateVel() to interpolate
/// @ref m_currSegmentVel towards the next point's velocity. Increments @ref m_segmentT by @ref
/// m_currSegmentVel and updates @ref m_curPos by linearly interpolating between the current and
/// next points based on the updated @ref m_segmentT. If @ref m_segmentT is less than or equal to
/// `1.0f`, then the interpolator is still in the current segment, so returns @ref
/// Status::InProgress. Otherwise, it proceeds to the next segment, checking to see if the
/// interpolator has reached the end of the rail. Updates @ref m_currVel, @ref m_curTangentDir, and
/// @ref m_currSegmentVel to reflect the new segment.
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

    if (hasReachedEndOfRail()) {
        status = Status::RailEnd;

        calcDirectionChange();
    }

    m_currVel = m_points[m_nextPointIdx].pos - m_points[m_currPointIdx].pos;
    m_curTangentDir = m_currVel;
    m_currSegmentVel = m_currSpeed / m_currVel.length();
    m_curTangentDir.normalise2();

    return status;
}

/// @addr{0x806F02EC}
/// @copydoc RailInterpolator::evalPositionAndTangentBehind()
/// @details Uses linear interpolation to get the position and tangent direction along the rail at a
/// specific `dist` distance behind the current position.
void RailLinearInterpolator::evalPositionAndTangentBehind(f32 dist, EGG::Vector3f &currPos,
        EGG::Vector3f &curTangentDir) const {
    s16 currIdx = 0;
    f32 t = 0.0f;

    getPathLocation(dist, currIdx, t);

    s16 nextIdx = currIdx + 1;
    if (nextIdx == m_pointCount) {
        nextIdx = 0;
    }

    currPos = lerp(t, currIdx, nextIdx);
    curTangentDir = m_transitions[currIdx].m_dir;
}

/// @addr{0x806F0610}
/// @brief Runs when the interpolator reaches the end of a segment
/// @details Updates @ref m_currPointIdx and @ref m_nextPointIdx to reflect the transition onto the
/// next segment, recalculates @ref m_currVel, and updates @ref m_segmentT to reflect the new
/// segment. If @ref m_usePerPointVelocities is `true`, @ref calcVelocities() will be called to
/// recalculate the per-point velocities.
/// @bug The game can access @ref m_points out-of-bounds, but then course-corrects by setting @ref
/// m_segmentT to `0.0f` once it detects an invalid @ref m_nextPointIdx, but after it already
/// performed an invalid read. To address this, we pre-emptively set @ref m_segmentT to `0.0f` and
/// later catch the invalid state in @ref calcVelocities() to avoid undefined behavior.
void RailLinearInterpolator::calcNextSegment() {
    calcNextIndices();

    if (hasReachedEndOfRail()) {
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

/// @addr{0x806EE924}
/// @copydoc RailInterpolator::init()
/// @details Sets @ref m_segmentT and @ref m_currPointIdx based on the provided `t` and `idx`
/// values respectively. Increments or decrements @ref m_nextPointIdx based on the direction of
/// travel and updates @ref m_forward accordingly. Updates @ref m_curTangentDir and @ref
/// m_currSegmentVel based on the new segment. Updates @ref m_curPos and @ref m_prevPos to reflect
/// the new segment. Sets @ref m_currSpeed, @ref m_currPointVel, and @ref m_nextPointVel to @ref
/// m_baseSpeed. Initializes @ref m_velocity to zero and resets @ref m_usePerPointVelocities to
/// `false`.
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
    m_currSpeed = m_baseSpeed;
    m_currPointVel = m_baseSpeed;
    m_nextPointVel = m_baseSpeed;
    m_velocity = 0.0f;
    m_usePerPointVelocities = false;
}

/// @addr{0x806EEBEC}
/// @copydoc RailInterpolator::calc()
/// @details If @ref m_usePerPointVelocities is `true`, then calls @ref updateVel() to interpolate
/// @ref m_currSegmentVel towards the next point's velocity. Evaluates the cubic bezier curve at the
/// current @ref m_segmentT, updating @ref m_curPos and @ref m_curTangentDir accordingly. Updates
/// @ref m_velocity to reflect the change in position and increments @ref m_segmentT by @ref
/// m_currSegmentVel. If @ref m_segmentT is less than or equal to `1.0f`, then the segment is still
/// in progress and early returns @ref State::InProgress. Otherwise, it proceeds to the next
/// segment, checking to see if the interpolator has reached the end of the rail. Updates @ref
/// m_currSegmentVel to reflect the new segment.
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

    if (hasReachedEndOfRail()) {
        status = Status::RailEnd;

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
/// @copydoc RailInterpolator::evalPositionAndTangentBehind()
/// @details Evaluates the cubic bezier curve to get the position and tangent direction at the
/// specific `dist` distance behind the current position.
void RailSmoothInterpolator::evalPositionAndTangentBehind(f32 dist, EGG::Vector3f &currPos,
        EGG::Vector3f &curTangentDir) const {
    s16 currIdx = 0;
    f32 t = 0.0f;

    getPathLocation(dist, currIdx, t);

    s16 nextIdx = currIdx + 1;
    if (nextIdx == m_pointCount) {
        nextIdx = 0;
    }

    t = m_forward ? calcT(t) : calcT(1.0f - t);

    calcCubicBezier(t, currIdx, nextIdx, currPos, curTangentDir);
}

/// @addr{0x806EF454}
/// @brief Evaluates the tangent direction of a cubic bezier curve at the given parameter `t`
/// @param t The parameter along the curve to evaluate, in the range `[0.0f, 1.0f]`
/// @param trans The bezier curve to evaluate
/// @return The tangent direction along the cubic bezier curve
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
/// @brief Maps a normalized position parameter `t` to a bezier curve parameter
/// @param t The position-based parameter to map, in the range `[0.0f, 1.0f]`
/// @return The mapped bezier curve parameter corresponding to the position-based parameter `t`
/// @details m_segmentT is a normalized position-based position along the current segment. A cubic
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
/// @details Updates the current and next point indices, updates @ref m_segmentT to reflect the new
/// segment, and updates per-point velocities, if applicable.
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
