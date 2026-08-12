#include "Rail.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806EC9A4}
Rail::Rail(u16 idx, System::MapdataPointInfo *info)
    : m_pointCount(info->pointCount()), m_isOscillating(info->setting(1) == 1), m_idx(idx) {
    m_points = info->points();
    m_hasCheckedCol = false;
}

/// @addr{0x806ECC40}
Rail::~Rail() = default;

/// @addr{0x806ECCC0}
/// @brief Checks for collision with the course geometry at each point in the rail and stores the
/// up vector of the colliding floor triangle, or @ref EGG::Vector3f::ey if no collision was found
void Rail::checkSphereFull() {
    if (m_hasCheckedCol) {
        return;
    }

    m_floorNrms = owning_span<EGG::Vector3f>(m_pointCount);

    for (size_t i = 0; i < m_pointCount; ++i) {
        CollisionInfo info;
        info.bbox.setZero();

        bool hasCourseCol = CollisionDirector::Instance()->checkSphereFull(100.0f, m_points[i].pos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &info, nullptr, 0);

        if (hasCourseCol) {
            if (info.floorDist > std::numeric_limits<f32>::min()) {
                m_floorNrms[i] = info.floorNrm;
            }
        } else {
            m_floorNrms[i] = EGG::Vector3f::ey;
        }
    }

    m_hasCheckedCol = true;
}

/// @addr{0x806EF9B4}
/// @brief Calculates each edge/transition's direction and length and computes the total rail length
RailLine::RailLine(u16 idx, System::MapdataPointInfo *info) : Rail(idx, info) {
    u16 transCount = m_isOscillating ? m_pointCount - 1 : m_pointCount;
    m_transitions = owning_span<RailLineTransition>(transCount);
    m_pathLength = 0.0f;

    for (u16 i = 0; i < m_pointCount - 1; ++i) {
        auto &transition = m_transitions[i];
        transition.m_dir = m_points[i + 1].pos - m_points[i].pos;
        transition.m_length = transition.m_dir.normalise();
        transition.m_lengthInv = 1.0f / transition.m_length;
        m_pathLength += transition.m_length;
    }

    if (!m_isOscillating) {
        auto &transition = m_transitions.back();
        transition.m_dir = m_points.front().pos - m_points.back().pos;
        transition.m_length = transition.m_dir.normalise();
        transition.m_lengthInv = 1.0f / transition.m_length;
        m_pathLength += transition.m_length;
    }
}

/// @addr{0x806EFD6C}
RailLine::~RailLine() = default;

/// @addr{0x806ED57C}
/// @brief Calculates each spline's control points and length and computes the total rail length
RailSpline::RailSpline(u16 idx, System::MapdataPointInfo *info) : Rail(idx, info) {
    u16 transitionCount = m_isOscillating ? m_pointCount - 1 : m_pointCount;
    m_transitions = owning_span<RailSplineTransition>(transitionCount);
    invalidateTransitions(false);

    m_pathLength = 0.0f;

    for (size_t i = 0; i < m_transitions.size(); ++i) {
        m_pathLength += m_transitions[i].m_length;
    }
}

/// @addr{0x806ED828}
RailSpline::~RailSpline() = default;

/// @addr{0x806EDA04}
/// @brief Computes the control points and lengths of each spline in the rail
void RailSpline::invalidateTransitions(bool lastOnly) {
    size_t count = ESTIMATOR_SAMPLE_COUNT * m_transitions.size() + 1;
    m_pathPercentages = owning_span<f32>(count);
    m_pathPercentageCount = 0;
    size_t transitionCount = m_transitions.size();

    if (m_isOscillating) {
        if (!lastOnly) {
            auto &firstTransition = m_transitions[0];
            firstTransition.m_p0 = m_points[0].pos;
            firstTransition.m_p1 =
                    (m_points[1].pos - m_points[0].pos).multInv(4.0f) + m_points[0].pos;
            firstTransition.m_p2 =
                    calcCubicBezierP2(m_points[0].pos, m_points[1].pos, m_points[2].pos);
            firstTransition.m_p3 = m_points[1].pos;
            firstTransition.m_length = estimateLength(firstTransition, ESTIMATOR_SAMPLE_COUNT);
            firstTransition.m_lengthInv = 1.0f / firstTransition.m_length;

            for (size_t i = 1; i < transitionCount - 1; ++i) {
                calcCubicBezierControlPoints(m_points[i - 1].pos, m_points[i].pos,
                        m_points[i + 1].pos, m_points[i + 2].pos, ESTIMATOR_SAMPLE_COUNT,
                        m_transitions[i]);
            }
        }

        auto &lastTransition = m_transitions.back();
        lastTransition.m_p0 = m_points[transitionCount - 1].pos;
        lastTransition.m_p1 = calcCubicBezierP1(m_points[transitionCount - 2].pos,
                m_points[transitionCount - 1].pos, m_points[transitionCount].pos);
        lastTransition.m_p2 =
                (m_points[transitionCount - 1].pos - m_points[transitionCount].pos).multInv(4.0f) +
                m_points[transitionCount].pos;
        lastTransition.m_p3 = m_points[transitionCount].pos;
        lastTransition.m_length = estimateLength(lastTransition, ESTIMATOR_SAMPLE_COUNT);
        lastTransition.m_lengthInv = 1.0f / lastTransition.m_length;
    } else {
        if (!lastOnly) {
            auto &firstTransition = m_transitions[0];
            firstTransition.m_p0 = m_points[0].pos;
            firstTransition.m_p1 = calcCubicBezierP1(m_points[transitionCount - 1].pos,
                    m_points[0].pos, m_points[1].pos);
            firstTransition.m_p2 =
                    calcCubicBezierP2(m_points[0].pos, m_points[1].pos, m_points[2].pos);
            firstTransition.m_p3 = m_points[1].pos;
            firstTransition.m_length = estimateLength(firstTransition, ESTIMATOR_SAMPLE_COUNT);
            firstTransition.m_lengthInv = 1.0f / firstTransition.m_length;

            for (size_t i = 1; i < transitionCount - 1; ++i) {
                if (i + 2 != transitionCount) {
                    calcCubicBezierControlPoints(m_points[i - 1].pos, m_points[i].pos,
                            m_points[i + 1].pos, m_points[i + 2].pos, ESTIMATOR_SAMPLE_COUNT,
                            m_transitions[i]);
                } else {
                    calcCubicBezierControlPoints(m_points[i - 1].pos, m_points[i].pos,
                            m_points[i + 1].pos, m_points[0].pos, ESTIMATOR_SAMPLE_COUNT,
                            m_transitions[i]);
                }
            }
        }

        auto &lastTransition = m_transitions.back();
        lastTransition.m_p0 = m_points[transitionCount - 1].pos;
        lastTransition.m_p1 = calcCubicBezierP1(m_points[transitionCount - 2].pos,
                m_points[transitionCount - 1].pos, m_points[0].pos);
        lastTransition.m_p2 = calcCubicBezierP2(m_points[transitionCount - 1].pos, m_points[0].pos,
                m_points[1].pos);
        lastTransition.m_p3 = m_points[0].pos;
        lastTransition.m_length = estimateLength(lastTransition, ESTIMATOR_SAMPLE_COUNT);
        lastTransition.m_lengthInv = 1.0f / lastTransition.m_length;
    }
}

/// @addr{0x806EE27C}
/// @brief Calculates the control points of a cubic bezier curve passing through the provided points
void RailSpline::calcCubicBezierControlPoints(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
        const EGG::Vector3f &p2, const EGG::Vector3f &p3, s32 count,
        RailSplineTransition &transition) {
    transition.m_p0 = p1;
    transition.m_p1 = calcCubicBezierP1(p0, p1, p2);
    transition.m_p2 = calcCubicBezierP2(p1, p2, p3);
    transition.m_p3 = p2;
    transition.m_length = estimateLength(transition, count);
    transition.m_lengthInv = 1.0f / transition.m_length;
}

/// @addr{0x806EE56C}
/// @brief Approximates the length of a cubic bezier curve by sampling points along the curve
/// @param count The number of samples to take along the curve (always @ref ESTIMATOR_SAMPLE_COUNT)
f32 RailSpline::estimateLength(const RailSplineTransition &transition, s32 count) {
    std::array<EGG::Vector3f, ESTIMATOR_SAMPLE_COUNT + 1> waypoints;

    for (s32 i = 0; i < count + 1; ++i) {
        waypoints[i] = cubicBezier(ESTIMATOR_STEP * static_cast<f32>(i), transition);
    }
    f32 length = 0.0f;

    // Numerator loop
    for (s32 i = 0; i < count; ++i) {
        m_pathPercentages[m_pathPercentageCount++] = length;
        length += (waypoints[i] - waypoints[i + 1]).length();
    }

    // Denominator loop
    for (s32 i = m_pathPercentageCount - 1; i > m_pathPercentageCount - count - 1; --i) {
        m_pathPercentages[i] /= length;
    }

    return length;
}

/// @addr{0x806EE408}
/// @brief Computes the outgoing bezier control point after p1
EGG::Vector3f RailSpline::calcCubicBezierP1(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
        const EGG::Vector3f &p2) const {
    EGG::Vector3f res = p2 - p0;
    f32 len = res.length();
    res.normalise2();
    return p1 + res * (len * CUBIC_BEZIER_TENSION_FACTOR);
}

/// @addr{0x806EE4B8}
/// @brief Computes the incoming bezier control point before p2
EGG::Vector3f RailSpline::calcCubicBezierP2(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
        const EGG::Vector3f &p2) const {
    EGG::Vector3f res = p0 - p2;
    f32 len = res.length();
    res.normalise2();
    return p1 + res * (len * CUBIC_BEZIER_TENSION_FACTOR);
}

/// @addr{0x806EE72C}
/// @brief Evaluates a cubic bezier curve at the given parameter t
/// @param t The parameter along the curve to evaluate, in the range [0, 1]
/// @param transition The bezier curve to evaluate
EGG::Vector3f RailSpline::cubicBezier(f32 t, const RailSplineTransition &transition) const {
    f32 dt = 1.0f - t;

    EGG::Vector3f res = transition.m_p0 * (dt * dt * dt);
    res += transition.m_p1 * (3.0f * t * (dt * dt));
    res += transition.m_p2 * (3.0f * (t * t) * dt);
    res += transition.m_p3 * (t * t * t);

    return res;
}

} // namespace Kinoko::Field
