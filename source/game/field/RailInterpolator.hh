#pragma once

#include "game/field/RailManager.hh"

namespace Kinoko::Field {

/// @brief Base class for interpolating along a rail. This class is responsible for calculating the
/// current position and tangent direction along the rail based on a given speed. It supports both
/// linear (@ref RailLinearInterpolator) and smooth (@ref RailSmoothInterpolator) interpolation
/// methods.
class RailInterpolator {
public:
    /// @brief Represents the status of the interpolation process
    enum class Status {
        InProgress = 0, ///< Currently interpolating along a segment
        SegmentEnd = 1, ///< Reached the end of a segment, ready to transition to the next
        RailEnd = 2,    ///< Reached the end of the rail
    };

    /// @addr{0x806ED160}
    /// @brief Constructor that initializes the rail interpolator with the provided speed and rail
    /// index
    /// @param speed The default speed of the interpolator
    /// @param idx The index of the rail to interpolate along
    RailInterpolator(f32 speed, u32 idx)
        : m_rail(RailManager::Instance()->rail(idx)),
          m_points(m_rail->points()),
          m_pointCount(static_cast<u16>(m_points.size())),
          m_isOscillating(m_rail->isOscillating()),
          m_baseSpeed(speed) {
        m_currSpeed = 0.0f;
    }

    /// @addr{0x806ED53C}
    /// @brief Default virtual destructor
    virtual ~RailInterpolator() = default;

    /// @brief Initializes the interpolator with a specific segment and position along that segment
    /// @param t The normalized position along the current segment [0.0, 1.0]
    /// @param idx The index of the current point in the rail's point list
    virtual void init(f32 t, u32 idx) = 0;

    /// @brief Calculates the next position and tangent direction along the rail
    /// @return The status of the interpolation after the calculation
    virtual Status calc() = 0;

    /// @brief Sets the current velocity that will be used for interpolation
    /// @param speed The new velocity to set
    virtual void setSpeed(f32 speed) = 0;

    /// @brief Returns the current velocity being used for interpolation
    /// @return The current velocity
    virtual f32 getCurrSpeed() = 0;

    /// @brief Fetches the position and tangent direction along the rail at a specific `dist`
    /// distance *behind* the current position
    /// @param dist The distance behind the current position along the rail
    /// @param currPos The calculated position along the rail at dist
    /// @param curTangentDir The calculated tangent direction along the rail at dist
    virtual void evalPositionAndTangentBehind(f32 dist, EGG::Vector3f &currPos,
            EGG::Vector3f &curTangentDir) const = 0;

    /// @brief Finds the segment of the rail that corresponds to a given distance dist`` behind the
    /// current position and calculates the normalized position along that segment
    /// @param dist The distance behind the current position along the rail
    /// @param idx The index of the segment that corresponds to `dist`
    /// @param t The normalized position along the segment that corresponds to `dist`
    virtual void getPathLocation(f32 dist, s16 &idx, f32 &t) const = 0;

    /// @brief The length of the current segment being interpolated along
    /// @return The length of the current segment
    [[nodiscard]] virtual f32 getCurrSegmentLength() const = 0;

    /// @addr{0x806ED204}
    /// @brief Reverses the direction of movement along the rail
    void reverseDirection() {
        m_forward = !m_forward;
        std::swap(m_currPointIdx, m_nextPointIdx);
        m_segmentT = 1.0f - m_segmentT;
    }

    /// @beginSetters

    /// @brief Sets whether to use per-point velocities for interpolation
    /// @param isSet `true` to use per-point velocities, `false` to use a constant velocity
    void setPerPointVelocities(bool isSet) {
        m_usePerPointVelocities = isSet;
    }

    /// @addr{0x806C63A8}
    /// @brief Sets the normalized position along the current segment
    /// @param t The normalized position along the current segment [0.0, 1.0]
    void setT(f32 t) {
        m_segmentT = t;
    }

    /// @endSetters

    /// @beginGetters

    /// @addr{0806ED24C}
    /// @brief Gets the up vector of the point at the given index
    /// @param idx The index of the point whose up vector is being retrieved
    /// @return The up vector of the specified point
    [[nodiscard]] const EGG::Vector3f &floorNrm(size_t idx) const {
        return m_rail->floorNrm(idx);
    }

    /// @addr{0x806ED30C}
    /// @brief Gets the sum of the lengths of the edges along the rail
    /// @return The sum of the lengths of the edges along the rail
    [[nodiscard]] f32 railLength() const {
        return m_rail->getPathLength();
    }

    /// @brief Gets the point that the rail interpolator is currently at or past
    /// @return The current point that the rail interpolator is at or past
    /// @pre Requires that @ref m_currPointIdx is a valid index within @ref m_points
    [[nodiscard]] const System::MapdataPointInfo::Point &curPoint() const {
        ASSERT(static_cast<size_t>(m_currPointIdx) < m_points.size());
        return m_points[m_currPointIdx];
    }

    /// @brief Gets the point that the rail interpolator is moving towards
    /// @return The next point that the rail interpolator is moving towards
    /// @pre Requires that @ref m_nextPointIdx is a valid index within @ref m_points
    [[nodiscard]] const System::MapdataPointInfo::Point &nextPoint() const {
        ASSERT(static_cast<size_t>(m_nextPointIdx) < m_points.size());
        return m_points[m_nextPointIdx];
    }

    /// @brief Gets the underlying rail that this class interpolates along
    /// @return A const pointer to the underlying rail
    [[nodiscard]] const Rail *rail() const {
        return m_rail;
    }

    /// @brief Gets the number of points along the rail
    /// @return The number of points along the rail
    [[nodiscard]] u16 pointCount() const {
        return m_pointCount;
    }

    /// @brief Gets the default speed of the rail interpolator
    /// @return The default speed of the rail interpolator
    [[nodiscard]] f32 speed() const {
        return m_baseSpeed;
    }

    /// @brief Gets the current position of the rail interpolator
    /// @return The current position of the rail interpolator
    [[nodiscard]] const EGG::Vector3f &curPos() const {
        return m_curPos;
    }

    /// @brief Gets the current tangent direction of the rail interpolator
    /// @return The current tangent direction of the rail interpolator
    [[nodiscard]] const EGG::Vector3f &curTangentDir() const {
        return m_curTangentDir;
    }

    /// @brief Gets the current speed of the rail interpolator
    /// @return The current speed of the rail interpolator
    [[nodiscard]] f32 currSpeed() const {
        return m_currSpeed;
    }

    /// @brief Gets the normalized position along the current segment
    /// @return The normalized position along the current segment [0.0, 1.0]
    [[nodiscard]] f32 segmentT() const {
        return m_segmentT;
    }

    /// @brief Checks if the movement direction is forward
    /// @return `true` if the movement direction is forward, `false` otherwise
    [[nodiscard]] bool isMovementDirectionForward() const {
        return m_forward;
    }

    /// @brief Gets the index of the current point
    /// @return The index of the current point within the rail's points
    [[nodiscard]] s16 curPointIdx() const {
        return m_currPointIdx;
    }

    /// @brief Gets the index of the next point
    /// @return The index of the next point within the rail's points
    [[nodiscard]] s16 nextPointIdx() const {
        return m_nextPointIdx;
    }

    /// @endGetters

protected:
    /// @addr{0x806ED3E4}
    /// @brief Updates the current velocity by linearly interpolating between the current and next
    /// point velocities based off the current @ref m_segmentT
    void updateVel() {
        setSpeed((1.0f - m_segmentT) * m_currPointVel + m_segmentT * m_nextPointVel);
    }

    void calcVelocities();

    /// @addr{0x806F0814}
    /// @brief Checks whether the interpolator has reached the end of the rail
    /// @return `true` if the interpolator has reached the end of the rail, `false` otherwise
    [[nodiscard]] bool hasReachedEndOfRail() const {
        if (!m_isOscillating) {
            return m_pointCount == m_nextPointIdx;
        }

        return m_forward ? m_nextPointIdx == m_pointCount : m_nextPointIdx == -1;
    }

    void calcDirectionChange();
    void calcNextIndices();

    template <typename T>
        requires std::is_same_v<T, RailLineTransition> || std::is_same_v<T, RailSplineTransition>
    void getPathLocationImpl(std::span<const T> transitions, f32 dist, s16 &idx, f32 &t) const;

    const Rail *const m_rail; ///< Pointer to the rail being interpolated along
    const std::span<const System::MapdataPointInfo::Point> m_points; ///< The points in the rail
    const u16 m_pointCount;        ///< The number of nodes/points in the rail
    const bool m_isOscillating;    ///< Whether the rail is a loop or a back-and-forth path
    const f32 m_baseSpeed;         ///< Default speed used when per-point velocities are not enabled
    bool m_usePerPointVelocities;  ///< When true, uses velocities defined at each point in the rail
    EGG::Vector3f m_curPos;        ///< The current position along the rail
    EGG::Vector3f m_curTangentDir; ///< The current tangent direction along the rail
    f32 m_currSpeed;    ///< Current speed either set via @ref setSpeed or from per-point velocities
    f32 m_currPointVel; ///< The velocity at the current point in the rail
    f32 m_nextPointVel; ///< The velocity at the next point in the rail
    f32 m_currSegmentVel; ///< The velocity along the current segment
    f32 m_segmentT;       ///< The normalized position along the current segment [0.0, 1.0]
    bool m_forward;       ///< Whether the interpolator is moving forward or backward along the rail
    s16 m_currPointIdx;   ///< The index of the current point in the rail's point list
    s16 m_nextPointIdx;   ///< The index of the next point in the rail's point list
};

/// @brief Interpolates along a rail using linear interpolation between points
class RailLinearInterpolator : public RailInterpolator {
public:
    /// @addr{0x806EFDC4}
    /// @brief Constructor that initializes the linear rail interpolator with the provided speed and
    /// rail index
    /// @param speed The default speed of the interpolator
    /// @param idx The index of the rail to interpolate along
    /// @pre Requires that `idx` corresponds to a @ref RailLine
    RailLinearInterpolator(f32 speed, u32 idx)
        : RailInterpolator(speed, idx),
          m_transitions(m_rail->getLinearTransitions()) {
        init(0.0f, 0);
    }

    /// @addr{0x806F094C}
    /// @brief Default virtual destructor
    ~RailLinearInterpolator() override = default;

    void init(f32 t, u32 idx) override;
    Status calc() override;

    /// @addr{0x806EFFF4}
    /// @copydoc RailInterpolator::setSpeed()
    /// @details Sets @ref m_currSpeed based on the provided `speed` and updates @ref
    /// m_currSegmentVel accordingly.
    void setSpeed(f32 speed) override {
        m_currSpeed = speed;
        m_currSegmentVel = m_currSpeed / m_currVel.length();
    }

    /// @addr{0x806F0944}
    /// @copybrief RailInterpolator::getCurrSpeed()
    /// @return The current speed of the interpolator, @ref m_currSpeed
    [[nodiscard]] f32 getCurrSpeed() override {
        return m_currSpeed;
    }

    void evalPositionAndTangentBehind(f32 dist, EGG::Vector3f &currPos,
            EGG::Vector3f &curTangentDir) const override;

    /// @addr{0x806F041C}
    /// @copydoc RailInterpolator::getPathLocation()
    void getPathLocation(f32 dist, s16 &idx, f32 &t) const override {
        getPathLocationImpl(m_transitions, dist, idx, t);
    }

    /// @addr{0x806F050C}
    /// @brief Gets the length of the current segment
    /// @return The length of the current segment
    [[nodiscard]] f32 getCurrSegmentLength() const override {
        s16 idx = m_forward ? m_currPointIdx : m_nextPointIdx;
        return m_transitions[idx].m_length;
    }

private:
    void calcNextSegment();

    /// @addr{0x806F0540}
    /// @brief Linearly interpolates between two points' positions based on `t`
    /// @param t The interpolation parameter, in the range `[0.0f, 1.0f]`
    /// @param currIdx The index of the current point
    /// @param nextIdx The index of the next point
    /// @return The interpolated position between the current and next points
    [[nodiscard]] EGG::Vector3f lerp(f32 t, u32 currIdx, u32 nextIdx) const {
        return m_points[currIdx].pos * (1.0f - t) + m_points[nextIdx].pos * t;
    }

    EGG::Vector3f m_currVel; ///< Position displacement from the current point to the next point
    const std::span<const RailLineTransition> m_transitions; ///< Transitions between rail points
};

/// @brief Interpolates along a rail using cubic bezier curves between points
class RailSmoothInterpolator : public RailInterpolator {
public:
    /// @addr{0x806EE830}
    /// @brief Constructor that initializes the smooth rail interpolator with the provided speed and
    /// rail index
    /// @param speed The default speed of the interpolator
    /// @param idx The index of the rail to interpolate along
    /// @pre Requires that `idx` corresponds to a @ref RailSpline
    RailSmoothInterpolator(f32 speed, u32 idx)
        : RailInterpolator(speed, idx),
          m_transitions(m_rail->getSplineTransitions()),
          m_estimatorSampleCount(static_cast<u32>(m_rail->getEstimatorSampleCount())),
          m_estimatorStep(m_rail->getEstimatorStep()),
          m_pathPercentages(m_rail->getPathPercentages()) {
        init(0.0f, 0);
    }

    /// @addr{0x806EF944}
    /// @brief Default virtual destructor
    ~RailSmoothInterpolator() override = default;

    void init(f32 t, u32 idx) override;
    Status calc() override;

    /// @addr{0x806EEB94}
    /// @copybrief RailInterpolator::setSpeed()
    /// @param speed The new speed to set for the interpolator
    /// @details Sets @ref m_currSpeed based on the provided `speed` and updates @ref
    /// m_currSegmentVel accordingly.
    void setSpeed(f32 speed) override {
        m_currSpeed = speed;
        s16 idx = m_forward ? m_currPointIdx : m_nextPointIdx;
        m_currSegmentVel = speed * m_transitions[idx].m_lengthInv;
    }

    /// @addr{0x806EF93C}
    /// @brief Gets the current speed of the interpolator
    /// @return The current speed of the interpolator, @ref m_velocity
    [[nodiscard]] f32 getCurrSpeed() override {
        return m_velocity;
    }

    void evalPositionAndTangentBehind(f32 dist, EGG::Vector3f &currPos,
            EGG::Vector3f &curTangentDir) const override;

    /// @addr{0x806EEFA0}
    /// @copydoc RailInterpolator::getPathLocation()
    void getPathLocation(f32 dist, s16 &idx, f32 &t) const override {
        getPathLocationImpl(m_transitions, dist, idx, t);
    }

    /// @addr{0x806EF09C}
    /// @brief Gets the length of the current segment
    /// @return The length of the current segment
    [[nodiscard]] f32 getCurrSegmentLength() const override {
        s16 idx = m_forward ? m_currPointIdx : m_nextPointIdx;
        return m_transitions[idx].m_length;
    }

private:
    /// @addr{0x806EF224}
    /// @brief Gets the position and tangent along the cubic bezier curve at a specific `t` value
    /// @param t The parameter along the curve to evaluate, in the range `[0.0f, 1.0f]`
    /// @param currIdx The index of the current point
    /// @param nextIdx The index of the next point
    /// @param pos Out parameter, the evaluated position along the curve
    /// @param dir Out parameter, the evaluated tangent direction along the curve

    void calcCubicBezier(f32 t, u32 currIdx, u32 nextIdx, EGG::Vector3f &pos,
            EGG::Vector3f &dir) const {
        const auto &transition = m_forward ? m_transitions[currIdx] : m_transitions[nextIdx];
        pos = calcCubicBezierPos(t, transition);
        dir = calcCubicBezierTangentDir(t, transition);
    }

    /// @addr{0x806EF350}
    /// @brief Evaluates a cubic bezier curve at the given parameter `t`
    /// @param t The parameter along the curve to evaluate, in the range `[0.0f, 1.0f]`
    /// @param trans The bezier curve to evaluate
    /// @return The evaluated position along the cubic bezier curve
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

    const std::span<const RailSplineTransition> m_transitions; ///< Transitions between rail points

    /// @brief The number of samples used to approximate the shape of the cubic bezier curves
    const u32 m_estimatorSampleCount;

    /// @brief The step size used to approximate the shape of the cubic bezier curves
    const f32 m_estimatorStep;

    /// @brief The percentage of the arc length at each sample point along the cubic bezier curves
    const std::span<const f32> m_pathPercentages;

    EGG::Vector3f m_prevPos; ///< The previous frame's position, used to calculate speed
    f32 m_velocity; ///< Magnitude of the diff between the current and previous frame's position
};

} // namespace Kinoko::Field
