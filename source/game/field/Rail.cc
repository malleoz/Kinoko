#include "Rail.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/KColData.hh"

namespace Kinoko::Field {

/// @addr{0x806EC9A4}
/// @brief Constructor that initializes the rail with the provided index and map data point info
/// @param idx The index of the rail
/// @param info Pointer to the map data point info containing the rail's points and settings
/// @details Sets @ref m_isOscillating based on whether `info` has setting 2 set to 1. Performs
/// a deep copy of the points from `info` and saves to @ref m_points. Saves the provided `idx`
/// to @ref m_idx. Finally, initializes @ref m_hasCheckedCol to `false`.
Rail::Rail(u16 idx, const System::MapdataPointInfo *info)
    : m_isOscillating(info->setting(1) == 1),
      m_points(info->points()),
      m_idx(idx) {
    m_hasCheckedCol = false;
}

/// @addr{0x806ECC40}
/// @brief Default virtual destructor
Rail::~Rail() = default;

/// @addr{0x806ECCC0}
/// @brief Checks for collision with the course geometry at each point in the rail and stores the
/// up vector of the colliding floor triangle, or @ref EGG::Vector3f::ey if no collision was found
/// @details If collision has already been checked (indicated via @ref m_hasCheckedCol), then this
/// function early returns. For each point along the rail, checks for #KCL_TYPE_FLOOR collision and
/// sets @ref m_floorNrms accordingly. Finally, sets @ref m_hasCheckedCol to prevent redundant
/// collision checks.
void Rail::checkSphereFull() {
    if (m_hasCheckedCol) {
        return;
    }

    m_floorNrms = owning_span<EGG::Vector3f>(m_points.size());

    for (size_t i = 0; i < m_points.size(); ++i) {
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

/// @brief Gets the number of points along the rail
/// @return The number of points in the rail, aka the size of @ref m_points
u16 Rail::pointCount() const {
    return m_points.size();
}

/// @brief Checks whether the rail is a loop or a back-and-forth path
/// @return @ref m_isOscillating which is `true` if the rail is oscillating, or `false` if the
/// rail is a loop
bool Rail::isOscillating() const {
    return m_isOscillating;
}

/// @brief Gets a read-only span of the points in the rail
/// @return A span of the points in the rail, from @ref m_points
std::span<const System::MapdataPointInfo::Point> Rail::points() const {
    return m_points.view();
}

/// @addr{0x806ED150}
/// @brief Gets the position of the point at the given index `idx
/// @param idx The index of the point in the rail
/// @return The position of the point at the given index in @ref m_points
/// @pre `idx` must be less than the number of points in the rail
const EGG::Vector3f &Rail::pointPos(u16 idx) const {
    ASSERT(idx < m_points.size());
    return m_points[idx].pos;
}

/// @brief Gets the up vector of the point at the given index `idx`
/// @param idx The index of the point in the rail
/// @return The up vector of the point at the given index `idx` in @ref m_floorNrms
/// @pre Assumes @ref checkSphereFull() has been called in order to allocate the array of floor
/// normals and set each element.
const EGG::Vector3f &Rail::floorNrm(u16 idx) const {
    ASSERT(!m_floorNrms.empty() && idx < m_floorNrms.size());
    return m_floorNrms[idx];
}

/// @addr{0x806F09C0} @addr{0x806EF9AC}
/// @brief Gets the length of the edge or curve between the two points
/// @return The length of the edge or curve between the two points, @ref m_pathLength
/// @details Normally @ref m_pathLength is a member in each derived class, but by moving it to
/// the base class, we can devirtualize this method.
f32 Rail::getPathLength() const {
    return m_pathLength;
}

/// @addr{0x806EF9B4}
/// @brief Constructor
/// @param idx The index of the rail
/// @param info Pointer to the map data point information
/// @details Initializes the rail line by calculating the direction and length between
/// points and the resulting total path length.
RailLine::RailLine(u16 idx, const System::MapdataPointInfo *info) : Rail(idx, info) {
    u16 transCount = m_isOscillating ? m_points.size() - 1 : m_points.size();
    m_transitions = owning_span<RailLineTransition>(transCount);
    m_pathLength = 0.0f;

    for (u16 i = 0; i < m_points.size() - 1; ++i) {
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

/// @addr{0x806F0994}
/// @copybrief Rail::getPathPercentages()
/// @return An empty span since this class does not use cubic bezier curves
/// @details In the base game we return a nullptr. To mimic this, return an empty span.
std::span<const f32> RailLine::getPathPercentages() const {
    static owning_span<f32> EMPTY_PERCENTAGES;
    return EMPTY_PERCENTAGES.view();
}

/// @addr{0x806F09B8}
/// @copybrief Rail:getLinearTransitions()
/// @return A span of the edges if this is a @ref RailLine
std::span<const RailLineTransition> RailLine::getLinearTransitions() const {
    return m_transitions.view();
}

/// @addr{0x806F09B0}
/// @copybrief Rail::getSplineTransitions()
/// @return An empty span since this class does not use cubic bezier curves
/// @details In the base game we return a nullptr. To mimic this, return an empty span.
std::span<const RailSplineTransition> RailLine::getSplineTransitions() const {
    static owning_span<RailSplineTransition> EMPTY_TRANSITIONS;
    return EMPTY_TRANSITIONS.view();
}

/// @addr{0x806ED57C}
/// @brief Constructor
/// @param idx The index of the rail
/// @param info Pointer to the map data point information
/// @details Calculates each spline's control points and length and computes the total rail length
RailSpline::RailSpline(u16 idx, const System::MapdataPointInfo *info) : Rail(idx, info) {
    u16 transitionCount = m_isOscillating ? m_points.size() - 1 : m_points.size();
    m_transitions = owning_span<RailSplineTransition>(transitionCount);
    invalidateTransitions(false);

    m_pathLength = 0.0f;

    for (size_t i = 0; i < m_transitions.size(); ++i) {
        m_pathLength += m_transitions[i].m_length;
    }
}

/// @addr{0x806ED828}
RailSpline::~RailSpline() = default;

/// @addr{0x806EF984}
/// @copybrief Rail::getPathPercentages()
/// @return A span of the path percentages for the cubic bezier curves
std::span<const f32> RailSpline::getPathPercentages() const {
    return m_pathPercentages.view();
}

/// @addr{0x806EF9A4}
/// @copybrief Rail::getLinearTransitions()
/// @return An empty span, since this class does not use linear transitions
/// @details In the base game we return a nullptr. To mimic this, return an empty span.
std::span<const RailLineTransition> RailSpline::getLinearTransitions() const {
    static owning_span<RailLineTransition> EMPTY_TRANSITIONS;
    return EMPTY_TRANSITIONS.view();
}

/// @addr{0x806EF99C}
/// @copybrief Rail::getSplineTransitions()
/// @return A span of the cubic bezier curves
std::span<const RailSplineTransition> RailSpline::getSplineTransitions() const {
    return m_transitions.view();
}

/// @addr{0x806EDA04}
/// @brief Computes the control points and lengths of each spline in the rail
/// @param lastOnly If `true`, only the last transition is recalculated
void RailSpline::invalidateTransitions(bool lastOnly) {
    size_t count = ESTIMATOR_SAMPLE_COUNT * m_transitions.size() + 1;
    m_pathPercentages = owning_span<f32>(count);
    m_pathPercentageCount = 0;
    size_t transitionCount = m_transitions.size();

    if (lastOnly) {
        m_transitions.back().resetTangentCoefficients();
    } else {
        for (auto &transition : m_transitions) {
            transition.resetTangentCoefficients();
        }
    }

    if (m_isOscillating) {
        if (!lastOnly) {
            auto &firstTransition = m_transitions[0];
            firstTransition.m_p0 = m_points[0].pos;
            firstTransition.m_p1 =
                    (m_points[1].pos - m_points[0].pos).multInv(4.0f) + m_points[0].pos;
            firstTransition.m_p2 =
                    CalcCubicBezierP2(m_points[0].pos, m_points[1].pos, m_points[2].pos);
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
        lastTransition.m_p1 = CalcCubicBezierP1(m_points[transitionCount - 2].pos,
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
            firstTransition.m_p1 = CalcCubicBezierP1(m_points[transitionCount - 1].pos,
                    m_points[0].pos, m_points[1].pos);
            firstTransition.m_p2 =
                    CalcCubicBezierP2(m_points[0].pos, m_points[1].pos, m_points[2].pos);
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
        lastTransition.m_p1 = CalcCubicBezierP1(m_points[transitionCount - 2].pos,
                m_points[transitionCount - 1].pos, m_points[0].pos);
        lastTransition.m_p2 = CalcCubicBezierP2(m_points[transitionCount - 1].pos, m_points[0].pos,
                m_points[1].pos);
        lastTransition.m_p3 = m_points[0].pos;
        lastTransition.m_length = estimateLength(lastTransition, ESTIMATOR_SAMPLE_COUNT);
        lastTransition.m_lengthInv = 1.0f / lastTransition.m_length;
    }
}

/// @addr{0x806EE27C}
/// @brief Calculates the control points of a cubic bezier curve passing through the provided
/// points
/// @param p0 The first point of the cubic bezier curve
/// @param p1 The second point of the cubic bezier curve
/// @param p2 The third point of the cubic bezier curve
/// @param p3 The fourth point of the cubic bezier curve
/// @param count The number of samples to use for length estimation
/// @param transition The RailSplineTransition object to store the calculated control points and
/// length
void RailSpline::calcCubicBezierControlPoints(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
        const EGG::Vector3f &p2, const EGG::Vector3f &p3, s32 count,
        RailSplineTransition &transition) {
    transition.m_p0 = p1;
    transition.m_p1 = CalcCubicBezierP1(p0, p1, p2);
    transition.m_p2 = CalcCubicBezierP2(p1, p2, p3);
    transition.m_p3 = p2;
    transition.m_length = estimateLength(transition, count);
    transition.m_lengthInv = 1.0f / transition.m_length;
}

/// @addr{0x806EE56C}
/// @brief Approximates the length of a cubic bezier curve by sampling points along the curve
/// @param transition The RailSplineTransition object representing the cubic bezier curve to
/// estimate the length of
/// @param count The number of samples to take along the curve (always @ref ESTIMATOR_SAMPLE_COUNT)
/// @return The estimated length of the cubic bezier curve
/// @details Computes the cubic bezier curve at `count` parameter values spaced evenly by @ref
/// ESTIMATOR_STEP. Sums the distances between consecutive sample points, accumulating the length up
/// to the sample point to @ref m_pathPercentages. After summing, divides each @ref
/// m_pathPercentages by the total length to normalize them. Finally, returns the estimated length.
f32 RailSpline::estimateLength(const RailSplineTransition &transition, s32 count) {
    std::array<EGG::Vector3f, ESTIMATOR_SAMPLE_COUNT + 1> waypoints;

    for (s32 i = 0; i < count + 1; ++i) {
        waypoints[i] = CubicBezier(ESTIMATOR_STEP * static_cast<f32>(i), transition);
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

/// @addr{0x806EE72C}
/// @brief Evaluates a cubic bezier curve at the given parameter t
/// @param t The parameter along the curve to evaluate, in the range [0, 1]
/// @param transition The bezier curve to evaluate
EGG::Vector3f RailSpline::CubicBezier(f32 t, const RailSplineTransition &transition) {
    f32 dt = 1.0f - t;

    EGG::Vector3f res = transition.m_p0 * (dt * dt * dt);
    res += transition.m_p1 * (3.0f * t * (dt * dt));
    res += transition.m_p2 * (3.0f * (t * t) * dt);
    res += transition.m_p3 * (t * t * t);

    return res;
}

/// @addr{0x806EE408}
/// @brief Computes the outgoing bezier control point after p1
/// @param p0 The first point of the cubic bezier curve
/// @param p1 The second point of the cubic bezier curve
/// @param p2 The third point of the cubic bezier curve
/// @return The computed outgoing bezier control point
EGG::Vector3f RailSpline::CalcCubicBezierP1(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
        const EGG::Vector3f &p2) {
    EGG::Vector3f res = p2 - p0;
    f32 len = res.length();
    res.normalise2();
    return p1 + res * (len * CUBIC_BEZIER_TENSION_FACTOR);
}

/// @addr{0x806EE4B8}
/// @brief Computes the incoming bezier control point before p2
/// @param p0 The first point of the cubic bezier curve
/// @param p1 The second point of the cubic bezier curve
/// @param p2 The third point of the cubic bezier curve
/// @return The computed incoming bezier control point
EGG::Vector3f RailSpline::CalcCubicBezierP2(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
        const EGG::Vector3f &p2) {
    EGG::Vector3f res = p0 - p2;
    f32 len = res.length();
    res.normalise2();
    return p1 + res * (len * CUBIC_BEZIER_TENSION_FACTOR);
}

} // namespace Kinoko::Field
