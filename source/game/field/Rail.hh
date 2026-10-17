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
    Rail(u16 idx, const System::MapdataPointInfo *info);
    virtual ~Rail();

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

    [[nodiscard]] u16 pointCount() const;
    [[nodiscard]] bool isOscillating() const;
    [[nodiscard]] std::span<const System::MapdataPointInfo::Point> points() const;
    [[nodiscard]] const EGG::Vector3f &pointPos(u16 idx) const;
    [[nodiscard]] const EGG::Vector3f &floorNrm(u16 idx) const;
    [[nodiscard]] f32 getPathLength() const;

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
class RailLine final : public Rail {
public:
    RailLine(u16 idx, const System::MapdataPointInfo *info);
    ~RailLine() override;

    [[nodiscard]] s32 getEstimatorSampleCount() const override;
    [[nodiscard]] f32 getEstimatorStep() const override;
    [[nodiscard]] std::span<const f32> getPathPercentages() const override;

private:
    [[nodiscard]] std::span<const RailLineTransition> getLinearTransitions() const override;
    [[nodiscard]] std::span<const RailSplineTransition> getSplineTransitions() const override;

    owning_span<RailLineTransition> m_transitions; ///< The edges between the points in the rail
};

/// @brief Represents a rail that is made up of cubic bezier curves between points
/// @details @ref RailSmoothInterpolator will fetch the spline transitions from this class so that
/// it can smoothly interpolate the position of an object along the rail based on its speed.
class RailSpline final : public Rail {
public:
    RailSpline(u16 idx, const System::MapdataPointInfo *info);
    ~RailSpline() override;

    [[nodiscard]] s32 getEstimatorSampleCount() const override;
    [[nodiscard]] f32 getEstimatorStep() const override;
    [[nodiscard]] std::span<const f32> getPathPercentages() const override;

private:
    [[nodiscard]] std::span<const RailLineTransition> getLinearTransitions() const override;
    [[nodiscard]] std::span<const RailSplineTransition> getSplineTransitions() const override;

    void invalidateTransitions(bool lastOnly);
    void calcCubicBezierControlPoints(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
            const EGG::Vector3f &p2, const EGG::Vector3f &p3, s32 count,
            RailSplineTransition &transition);
    [[nodiscard]] f32 estimateLength(const RailSplineTransition &transition, s32 count);

    owning_span<RailSplineTransition> m_transitions; ///< The splines between the points in the rail

    /// @brief The percentage of the arc length at each sample point along the cubic bezier curves
    owning_span<f32> m_pathPercentages;

    /// @brief The number of sampled path percentages stored in @ref m_pathPercentages
    s32 m_pathPercentageCount;

    [[nodiscard]] static EGG::Vector3f CubicBezier(f32 t, const RailSplineTransition &transition);
    [[nodiscard]] static EGG::Vector3f CalcCubicBezierP1(const EGG::Vector3f &p0,
            const EGG::Vector3f &p1, const EGG::Vector3f &p2);
    [[nodiscard]] static EGG::Vector3f CalcCubicBezierP2(const EGG::Vector3f &p0,
            const EGG::Vector3f &p1, const EGG::Vector3f &p2);

    /// @brief Scales how far the control points are pulled out from the curve's anchor points
    static constexpr f32 CUBIC_BEZIER_TENSION_FACTOR = 0.1f;

    /// @brief The number of samples used to approximate the shape of the cubic bezier curves
    static constexpr u32 ESTIMATOR_SAMPLE_COUNT = 10;

    /// @brief The step size used to approximate the shape of the cubic bezier curves
    static constexpr f32 ESTIMATOR_STEP = 1.0f / static_cast<f32>(ESTIMATOR_SAMPLE_COUNT);
};

} // namespace Kinoko::Field
