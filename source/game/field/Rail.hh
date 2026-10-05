#pragma once

#include "game/system/map/MapdataPointInfo.hh"

namespace Kinoko::Field {

/// @brief Represents the edge between two points in a @ref Rail
/// @details Each edge has a direction and a length. The direction is a unit vector pointing from
/// the first point to the second point.
struct RailLineTransition {
    f32 m_length;        ///< The length of the edge between the two points
    f32 m_lengthInv;     ///< The inverse of the length of the edge between the two points
    EGG::Vector3f m_dir; ///< The direction from the first point to the second point
};

/// @brief Represents a cubic bezier curve between two points in a @ref Rail
/// @details Each cubic bezier curve has four control points. The curve starts at @ref m_p0 and ends
/// at @ref m_p3. The control points @ref m_p1 and @ref m_p2 define the shape of the curve.
struct RailSplineTransition {
    EGG::Vector3f m_p0; ///< The first control point (start point))
    EGG::Vector3f m_p1; ///< The second control point
    EGG::Vector3f m_p2; ///< The third control point
    EGG::Vector3f m_p3; ///< The fourth control point (end point)
    f32 m_length;       ///< The length of the curve between the two points
    f32 m_lengthInv;    ///< The inverse of the length of the curve between the two points
};

/// @brief Represents a series of points in 3D space that can be used to define a path
/// @details A rail can be either a series of straight lines (defined by @ref RailLine) or a series
/// of cubic bezier curves (defined by @ref RailSpline). Upon reaching the end of a rail, the rail
/// can either reset back to the first point, or it can reverse direction. Rails originate from
/// "course.kmp" and are cached in @ref RailManager. Objects can be mapped to a particular rail so
/// that they can move along the path defined by the rail. On every frame, objects can interface
/// with a @ref RailInterpolator, which calculates the position of an object along a rail based on
/// its speed.
class Rail {
public:
    /// @addr{0x806EC9A4}
    /// @brief Constructor that initializes the rail with the provided index and map data point info
    /// @param idx The index of the rail
    /// @param info Pointer to the map data point info containing the rail's points and settings
    /// @details Sets @ref m_isOscillating based on whether `info` has setting 2 set to 1. Performs
    /// a deep copy of the points from `info` and saves to @ref m_points. Saves the provided `idx`
    /// to @ref m_idx. Finally, initializes @ref m_hasCheckedCol to `false`.
    Rail(u16 idx, System::MapdataPointInfo *info)
        : m_isOscillating(info->setting(1) == 1),
          m_points(info->points()),
          m_idx(idx) {
        m_hasCheckedCol = false;
    }

    /// @addr{0x806ECC40}
    /// @brief Default virtual destructor
    virtual ~Rail() = default;

    /// @brief Gets the edges between the points in the rail
    /// @return A span of the edges if this is a @ref RailLine, or an empty span if this is a @ref
    /// RailSpline
    virtual std::span<const RailLineTransition> getLinearTransitions() const = 0;

    /// @brief Gets the cubic bezier curves between the points in the rail
    /// @return A span of the cubic bezier curves if this is a @ref RailSpline, or an empty span if
    /// this is a @ref RailLine
    virtual std::span<const RailSplineTransition> getSplineTransitions() const = 0;

    /// @brief The number of samples used to approximate the shape of the cubic bezier curves
    /// @return The number of samples if this is a @ref RailSpline, or 0 if this is a @ref RailLine
    virtual s32 getEstimatorSampleCount() const = 0;

    /// @brief Inverse of the sample count used to approximate the shape of the cubic bezier curves
    /// @return The step size if this is a @ref RailSpline, or 0 if this is a @ref RailLine
    virtual f32 getEstimatorStep() const = 0;

    /// @brief The percentage of the arc length at each sample point along the cubic bezier curves
    /// @return A span of the percentages if this is a @ref RailSpline, or an empty span if this is
    /// a @ref RailLine
    virtual std::span<const f32> getPathPercentages() const = 0;

    void checkSphereFull();

    /// @beginGetters

    /// @brief Gets the number of points along the rail
    /// @return The number of points in the rail
    [[nodiscard]] u16 pointCount() const {
        return m_points.size();
    }

    /// @brief Checks whether the rail is a loop or a back-and-forth path
    /// @return `true` if the rail is oscillating, `false` if the rail is a loop
    [[nodiscard]] bool isOscillating() const {
        return m_isOscillating;
    }

    /// @brief Gets a read-only span of the points in the rail
    /// @return A span of the points in the rail
    [[nodiscard]] std::span<const System::MapdataPointInfo::Point> points() const {
        return m_points.view();
    }

    /// @addr{0x806ED150}
    /// @brief Gets the position of the point at the given index `idx
    /// @param idx The index of the point in the rail
    /// @return The position of the point at the given index
    /// @pre `idx` must be less than the number of points in the rail
    [[nodiscard]] const EGG::Vector3f &pointPos(u16 idx) const {
        ASSERT(idx < m_points.size());
        return m_points[idx].pos;
    }

    /// @brief Gets the up vector of the point at the given index `idx`
    /// @param idx The index of the point in the rail
    /// @return The up vector of the point at the given index `idx`
    /// @pre Assumes @ref checkSphereFull() has been called in order to allocate the array of floor
    /// normals and set each element.
    [[nodiscard]] const EGG::Vector3f &floorNrm(u16 idx) const {
        ASSERT(!m_floorNrms.empty() && idx < m_floorNrms.size());
        return m_floorNrms[idx];
    }

    /// @addr{0x806F09C0} @addr{0x806EF9AC}
    /// @brief Gets the length of the edge or curve between the two points
    /// @return The length of the edge or curve between the two points
    /// @details Normally @ref m_pathLength is a member in each derived class, but by moving it to
    /// the base class, we can devirtualize this method.
    [[nodiscard]] f32 getPathLength() const {
        return m_pathLength;
    }

    /// @endGetters

protected:
    const bool m_isOscillating; ///< Whether the rail is a loop or a back-and-forth path
    const owning_span<System::MapdataPointInfo::Point> m_points; ///< The points in the rail

    /// @brief The sum of the lengths of the edges along the rail
    /// @details This is normally defined as a member in each derived class, but we move it to the
    /// base class so we can devirtualize @ref getPathLength().
    f32 m_pathLength;

private:
    const u16 m_idx;      ///< The index of the rail in the @ref System::CourseMap's point info list
    bool m_hasCheckedCol; ///< Whether the rail has checked for collision with the course geometry
    owning_span<EGG::Vector3f> m_floorNrms; ///< The up vectors of the points in the rail
};

/// @brief Represents a rail that is made up of straight lines between points
/// @details @ref RailLinearInterpolator will fetch the linear transitions from this class so that
/// it can linearly interpolate the position of an object along the rail based on its speed.
class RailLine : public Rail {
public:
    RailLine(u16 idx, System::MapdataPointInfo *info);

    /// @addr{0x806EFD6C}
    ~RailLine() override = default;

    /// @addr{0x806F09A8}
    /// @copybrief Rail::getEstimatorSampleCount()
    /// @return `0` since this class does not use cubic bezier curves
    [[nodiscard]] s32 getEstimatorSampleCount() const override {
        return 0;
    }

    /// @addr{0x806F099C}
    /// @copybrief Rail::getEstimatorStep()
    /// @return `0.0f` since this class does not use cubic bezier curves
    [[nodiscard]] f32 getEstimatorStep() const override {
        return 0.0f;
    }

    /// @addr{0x806F0994}
    /// @copybrief Rail::getPathPercentages()
    /// @return An empty span since this class does not use cubic bezier curves
    /// @details In the base game we return a nullptr. To mimic this, return an empty span.
    [[nodiscard]] std::span<const f32> getPathPercentages() const override {
        static owning_span<f32> EMPTY_PERCENTAGES;
        return EMPTY_PERCENTAGES.view();
    }

private:
    /// @addr{0x806F09B8}
    /// @copybrief Rail:getLinearTransitions()
    /// @return A span of the edges if this is a @ref RailLine
    [[nodiscard]] std::span<const RailLineTransition> getLinearTransitions() const override {
        return m_transitions.view();
    }

    /// @addr{0x806F09B0}
    /// @copybrief Rail::getSplineTransitions()
    /// @return An empty span since this class does not use cubic bezier curves
    /// @details In the base game we return a nullptr. To mimic this, return an empty span.
    [[nodiscard]] std::span<const RailSplineTransition> getSplineTransitions() const override {
        static owning_span<RailSplineTransition> EMPTY_TRANSITIONS;
        return EMPTY_TRANSITIONS.view();
    }

    owning_span<RailLineTransition> m_transitions; ///< The edges between the points in the rail
};

/// @brief Represents a rail that is made up of cubic bezier curves between points
/// @details @ref RailSmoothInterpolator will fetch the spline transitions from this class so that
/// it can smoothly interpolate the position of an object along the rail based on its speed.
class RailSpline : public Rail {
public:
    RailSpline(u16 idx, System::MapdataPointInfo *info);

    /// @addr{0x806ED828}
    ~RailSpline() override = default;

    /// @addr{0x806EF994}
    /// @copybrief Rail::getEstimatorSampleCount()
    /// @return The number of samples
    [[nodiscard]] s32 getEstimatorSampleCount() const override {
        return ESTIMATOR_SAMPLE_COUNT;
    }

    /// @addr{0x806EF98C}
    /// @copybrief Rail::getEstimatorStep()
    /// @return The step size for the estimator
    [[nodiscard]] f32 getEstimatorStep() const override {
        return ESTIMATOR_STEP;
    }

    /// @addr{0x806EF984}
    /// @copybrief Rail::getPathPercentages()
    /// @return A span of the path percentages for the cubic bezier curves
    [[nodiscard]] std::span<const f32> getPathPercentages() const override {
        return m_pathPercentages.view();
    }

private:
    /// @addr{0x806EF9A4}
    /// @copybrief Rail::getLinearTransitions()
    /// @return An empty span, since this class does not use linear transitions
    /// @details In the base game we return a nullptr. To mimic this, return an empty span.
    [[nodiscard]] std::span<const RailLineTransition> getLinearTransitions() const override {
        static owning_span<RailLineTransition> EMPTY_TRANSITIONS;
        return EMPTY_TRANSITIONS.view();
    }

    /// @addr{0x806EF99C}
    /// @copybrief Rail::getSplineTransitions()
    /// @return A span of the cubic bezier curves
    [[nodiscard]] std::span<const RailSplineTransition> getSplineTransitions() const override {
        return m_transitions.view();
    }

    void invalidateTransitions(bool lastOnly);

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
    void calcCubicBezierControlPoints(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
            const EGG::Vector3f &p2, const EGG::Vector3f &p3, s32 count,
            RailSplineTransition &transition) {
        transition.m_p0 = p1;
        transition.m_p1 = calcCubicBezierP1(p0, p1, p2);
        transition.m_p2 = calcCubicBezierP2(p1, p2, p3);
        transition.m_p3 = p2;
        transition.m_length = estimateLength(transition, count);
        transition.m_lengthInv = 1.0f / transition.m_length;
    }

    [[nodiscard]] f32 estimateLength(const RailSplineTransition &transition, s32 count);

    /// @addr{0x806EE408}
    /// @brief Computes the outgoing bezier control point after p1
    /// @param p0 The first point of the cubic bezier curve
    /// @param p1 The second point of the cubic bezier curve
    /// @param p2 The third point of the cubic bezier curve
    /// @return The computed outgoing bezier control point
    [[nodiscard]] EGG::Vector3f calcCubicBezierP1(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
            const EGG::Vector3f &p2) const {
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
    [[nodiscard]] EGG::Vector3f calcCubicBezierP2(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
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
    [[nodiscard]] EGG::Vector3f cubicBezier(f32 t, const RailSplineTransition &transition) const {
        f32 dt = 1.0f - t;

        EGG::Vector3f res = transition.m_p0 * (dt * dt * dt);
        res += transition.m_p1 * (3.0f * t * (dt * dt));
        res += transition.m_p2 * (3.0f * (t * t) * dt);
        res += transition.m_p3 * (t * t * t);

        return res;
    }

    owning_span<RailSplineTransition> m_transitions; ///< The splines between the points in the rail

    /// @brief The percentage of the arc length at each sample point along the cubic bezier curves
    owning_span<f32> m_pathPercentages;

    /// @brief The number of sampled path percentages stored in @ref m_pathPercentages
    s32 m_pathPercentageCount;

    /// @brief Scales how far the control points are pulled out from the curve's anchor points
    static constexpr f32 CUBIC_BEZIER_TENSION_FACTOR = 0.1f;

    /// @brief The number of samples used to approximate the shape of the cubic bezier curves
    static constexpr u32 ESTIMATOR_SAMPLE_COUNT = 10;

    /// @brief The step size used to approximate the shape of the cubic bezier curves
    static constexpr f32 ESTIMATOR_STEP = 1.0f / static_cast<f32>(ESTIMATOR_SAMPLE_COUNT);
};

} // namespace Kinoko::Field
