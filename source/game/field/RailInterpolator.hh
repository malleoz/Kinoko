#pragma once

#include "game/field/Rail.hh"

namespace Kinoko::Field {

/// @brief Base class for interpolating along a rail. This class is responsible for calculating the
/// current position and tangent direction along the rail based on a given speed. It supports both
/// linear and smooth interpolation methods.
class RailInterpolator {
public:
    /// @brief Represents the status of the interpolation process
    enum class Status {
        InProgress = 0,        ///< Currently interpolating along a segment
        SegmentEnd = 1,        ///< Reached the end of a segment, ready to transition to the next
        ChangingDirection = 2, ///< Reached the end of the rail and is changing direction (for
                               ///< oscillating rails)
    };

    RailInterpolator(f32 speed, u32 idx);
    virtual ~RailInterpolator();

    /// @brief Initializes the interpolator with a specific segment and position along that segment
    /// @param t The normalized position along the current segment [0.0, 1.0]
    /// @param idx The index of the current point in the rail's point list
    virtual void init(f32 t, u32 idx) = 0;

    /// @brief Calculates the next position and tangent direction along the rail
    /// @return The status of the interpolation after the calculation
    virtual Status calc() = 0;

    /// @brief Sets the current velocity that will be used for interpolation
    /// @param speed The new velocity to set
    virtual void setCurrVel(f32 speed) = 0;

    /// @brief Returns the current velocity being used for interpolation
    /// @return The current velocity
    virtual f32 getCurrVel() = 0;

    /// @brief Fetches the position and tangent direction along the rail at a specific t distance
    /// *behind* the current position
    /// @param t The distance behind the current position along the rail
    /// @param currPos The calculated position along the rail at t
    /// @param curTangentDir The calculated tangent direction along the rail at t
    virtual void evalPositionAndTangentBehind(f32 t, EGG::Vector3f &currPos,
            EGG::Vector3f &curTangentDir) const = 0;

    /// @brief Finds the segment of the rail that corresponds to a given distance t behind the
    /// current position and calculates the normalized position along that segment
    /// @param t The distance behind the current position along the rail
    /// @param idx The index of the segment that corresponds to t
    /// @param len The normalized position along the segment that corresponds to t
    virtual void getPathLocation(f32 t, s16 &idx, f32 &len) const = 0;

    /// @brief The length of the current segment being interpolated along
    /// @return The length of the current segment
    [[nodiscard]] virtual f32 getCurrSegmentLength() const = 0;

    /// @beginSetters
    /// @brief Sets whether to use per-point velocities for interpolation
    /// @param isSet True to use per-point velocities, false to use a constant velocity
    void setPerPointVelocities(bool isSet) {
        m_usePerPointVelocities = isSet;
    }

    /// @addr{0x806C63A8}
    void setT(f32 t) {
        m_segmentT = t;
    }
    /// @endSetters

    /// @addr{0x806ED204}
    /// @brief Reverses the direction of movement along the rail. This is used for oscillating rails
    void reverseDirection() {
        m_forward = !m_forward;
        std::swap(m_currPointIdx, m_nextPointIdx);
        m_segmentT = 1.0f - m_segmentT;
    }

    /// @beginGetters
    [[nodiscard]] const EGG::Vector3f &floorNrm(size_t idx) const;
    [[nodiscard]] f32 railLength() const;

    [[nodiscard]] const System::MapdataPointInfo::Point &curPoint() const {
        ASSERT(static_cast<size_t>(m_currPointIdx) < m_points.size());
        return m_points[m_currPointIdx];
    }

    [[nodiscard]] const System::MapdataPointInfo::Point &nextPoint() const {
        ASSERT(static_cast<size_t>(m_nextPointIdx) < m_points.size());
        return m_points[m_nextPointIdx];
    }

    [[nodiscard]] s16 railIdx() const {
        return m_railIdx;
    }

    [[nodiscard]] u16 pointCount() const {
        return m_pointCount;
    }

    [[nodiscard]] f32 speed() const {
        return m_speed;
    }

    [[nodiscard]] const EGG::Vector3f &curPos() const {
        return m_curPos;
    }

    [[nodiscard]] const EGG::Vector3f &curTangentDir() const {
        return m_curTangentDir;
    }

    [[nodiscard]] f32 currVel() const {
        return m_currSpeed;
    }

    [[nodiscard]] f32 segmentT() const {
        return m_segmentT;
    }

    [[nodiscard]] bool isMovementDirectionForward() const {
        return m_forward;
    }

    [[nodiscard]] s16 curPointIdx() const {
        return m_currPointIdx;
    }

    [[nodiscard]] s16 nextPointIdx() const {
        return m_nextPointIdx;
    }
    /// @endGetters

protected:
    /// @addr{0x806ED3E4}
    /// @brief Updates the current velocity by linearly interpolating between the current and next
    /// point velocities
    void updateVel() {
        f32 t = m_segmentT;
        setCurrVel((1.0f - t) * m_currPointVel + t * m_nextPointVel);
    }

    void calcVelocities();

    /// @addr{0x806F0814}
    /// @brief Checks whether the interpolator has reached the end of the rail
    [[nodiscard]] bool shouldChangeDirection() const {
        if (!m_isOscillating) {
            return m_pointCount == m_nextPointIdx;
        }

        return m_forward ? m_nextPointIdx == m_pointCount : m_nextPointIdx == -1;
    }

    void calcDirectionChange();
    void calcNextIndices();

    const s16 m_railIdx; ///< The index of the rail in the @ref RailManager
    u16 m_pointCount;    ///< The number of nodes/points in the rail
    const std::span<const System::MapdataPointInfo::Point> m_points; ///< The points in the rail
    bool m_isOscillating;          ///< Whether the rail is a loop or a back-and-forth path
    const f32 m_speed;             ///< Default speed used when per-point velocities are not enabled
    bool m_usePerPointVelocities;  ///< When true, uses velocities defined at each point in the rail
    EGG::Vector3f m_curPos;        ///< The current position along the rail
    EGG::Vector3f m_curTangentDir; ///< The current tangent direction along the rail
    f32 m_currSpeed; ///< Current speed either set via @ref setCurrVel or from per-point velocities
    f32 m_currPointVel;   ///< The velocity at the current point in the rail
    f32 m_nextPointVel;   ///< The velocity at the next point in the rail
    f32 m_currSegmentVel; ///< The velocity along the current segment
    f32 m_segmentT;       ///< The normalized position along the current segment [0.0, 1.0]
    bool m_forward;       ///< Whether the interpolator is moving forward or backward along the rail
    s16 m_currPointIdx;   ///< The index of the current point in the rail's point list
    s16 m_nextPointIdx;   ///< The index of the next point in the rail's point list
};

/// @brief Interpolates along a rail using linear interpolation between points
class RailLinearInterpolator : public RailInterpolator {
public:
    RailLinearInterpolator(f32 speed, u32 idx);
    ~RailLinearInterpolator() override;

    void init(f32 t, u32 idx) override;
    Status calc() override;

    /// @addr{0x806EFFF4}
    void setCurrVel(f32 speed) override {
        m_currSpeed = speed;
        m_currSegmentVel = m_currSpeed / m_currVel.length();
    }

    /// @addr{0x806F0944}
    [[nodiscard]] f32 getCurrVel() override {
        return m_currSpeed;
    }

    void evalPositionAndTangentBehind(f32 t, EGG::Vector3f &currPos,
            EGG::Vector3f &curTangentDir) const override;
    void getPathLocation(f32 t, s16 &idx, f32 &len) const override;

    /// @addr{0x806F050C}
    [[nodiscard]] f32 getCurrSegmentLength() const override {
        s16 idx = m_forward ? m_currPointIdx : m_nextPointIdx;
        return m_transitions[idx].m_length;
    }

private:
    void calcNextSegment();

    /// @addr{0x806F0540}
    /// @brief Linearly interpolates between the current and next point positions based on t
    [[nodiscard]] EGG::Vector3f lerp(f32 t, u32 currIdx, u32 nextIdx) const {
        return m_points[currIdx].pos * (1.0f - t) + m_points[nextIdx].pos * t;
    }

    EGG::Vector3f m_currVel; ///< Position displacement from the current point to the next point
    std::span<const RailLineTransition> m_transitions; ///< The transitions between rail points
};

/// @brief Interpolates along a rail using cubic bezier curves between points
class RailSmoothInterpolator : public RailInterpolator {
public:
    RailSmoothInterpolator(f32 speed, u32 idx);
    ~RailSmoothInterpolator() override;

    void init(f32 t, u32 idx) override;
    Status calc() override;

    /// @addr{0x806EEB94}
    void setCurrVel(f32 speed) override {
        m_currSpeed = speed;
        s16 idx = m_forward ? m_currPointIdx : m_nextPointIdx;
        m_currSegmentVel = speed * m_transitions[idx].m_lengthInv;
    }

    /// @addr{0x806EF93C}
    [[nodiscard]] f32 getCurrVel() override {
        return m_speed;
    }

    void evalPositionAndTangentBehind(f32 t, EGG::Vector3f &currPos,
            EGG::Vector3f &curTangentDir) const override;
    void getPathLocation(f32 t, s16 &idx, f32 &len) const override;

    /// @addr{0x806EF09C}
    [[nodiscard]] f32 getCurrSegmentLength() const override {
        s16 idx = m_forward ? m_currPointIdx : m_nextPointIdx;
        return m_transitions[idx].m_length;
    }

private:
    /// @addr{0x806EF224}
    /// @brief Gets the position and tangent along the cubic bezier curve at a specific t value
    void calcCubicBezier(f32 t, u32 currIdx, u32 nextIdx, EGG::Vector3f &pos,
            EGG::Vector3f &dir) const {
        const auto &transition = m_forward ? m_transitions[currIdx] : m_transitions[nextIdx];
        pos = calcCubicBezierPos(t, transition);
        dir = calcCubicBezierTangentDir(t, transition);
    }

    /// @addr{0x806EF350}
    /// @brief Evaluates a cubic bezier curve at the given parameter t
    /// @param t The parameter along the curve to evaluate, in the range [0, 1]
    /// @param trans The bezier curve to evaluate
    [[nodiscard]] EGG::Vector3f calcCubicBezierPos(f32 t, const RailSplineTransition &trans) const {
        f32 dt = 1.0f - t;

        EGG::Vector3f res = trans.m_p0 * (dt * dt * dt);
        res += trans.m_p1 * (3.0f * t * (dt * dt));
        res += trans.m_p2 * (3.0f * (t * t) * dt);
        res += trans.m_p3 * (t * t * t);

        return res;
    }

    [[nodiscard]] EGG::Vector3f calcCubicBezierTangentDir(f32 t,
            const RailSplineTransition &trans) const;
    [[nodiscard]] f32 calcT(f32 t) const;
    void calcNextSegment();

    std::span<const RailSplineTransition> m_transitions; ///< The transitions between rail points

    /// @brief The number of samples used to approximate the shape of the cubic bezier curves
    u32 m_estimatorSampleCount;

    /// @brief The step size used to approximate the shape of the cubic bezier curves
    f32 m_estimatorStep;

    /// @brief The percentage of the arc length at each sample point along the cubic bezier curves
    std::span<const f32> m_pathPercentages;

    EGG::Vector3f m_prevPos; ///< The previous frame's position, used to calculate speed
    f32 m_speed; ///< Magnitude of the difference between the current and previous frame's position
};

} // namespace Kinoko::Field
