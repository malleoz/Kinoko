#include "KartPullPath.hh"

#include "game/system/CourseMap.hh"
#include "game/system/map/MapdataArea.hh"
#include "game/system/map/MapdataPointInfo.hh"

#include <egg/geom/Plane.hh>

namespace Kinoko::Kart {

/// @addr{0x8059308C}
/// @brief Constructor
/// @param handle Pointer to the owning @ref KartPullPath subsystem
/// @param type The type of tracker to use for the pull path
/// @details Sets @ref m_type and @ref m_handle to the provided `type` and `handle`. Initializes
/// @ref m_currentIdx to zero and @ref m_pointInfo to `nullptr`.
KartPullPathTracker::KartPullPathTracker(KartPullPath *handle, Type type)
    : m_type(type),
      m_currentIdx(0),
      m_pointInfo(nullptr),
      m_handle(handle) {}

/// @addr{0x805930E4}
/// @brief Default virtual destructor
KartPullPathTracker::~KartPullPathTracker() = default;

/// @brief Calls into the appropriate tracker type's calc function to find the current segment
/// of the pull path
/// @details If this is a @ref Type::Global tracker, calls @ref calcTrackerGlobal() to perform
/// an exhaustive round-robin scan of all points in the pyll path. Otherwise if this is a @ref
/// Type::Regional tracker, calls @ref calcTrackerRegional() to check the current, next, and
/// previous segments.
/// @pre Requires that @ref setPointInfo() has been set to initialize @ref m_pointInfo.
void KartPullPathTracker::calc() {
    ASSERT(m_pointInfo);
    if (m_type == Type::Global) {
        calcTrackerGlobal();
    } else if (m_type == Type::Regional) {
        calcTrackerRegional();
    }
}

/// @brief Sets the current index into the pull path's array of points
/// @param idx The index to set as the current point in the pull path
void KartPullPathTracker::setCurrentIdx(s16 idx) {
    m_currentIdx = idx;
}

/// @brief Sets the current point info for the tracker to search through
/// @param info The point info to set for the tracker
/// @details Sets @ref m_pointInfo to the provided `info` and resets @ref m_currentIdx to zero.
void KartPullPathTracker::setPointInfo(const System::MapdataPointInfo *info) {
    m_pointInfo = info;
    m_currentIdx = 0;
}

/// @addr{0x80593138}
/// @brief Checks to see if the kart is within the global tracker's current segment
/// @details The global tracker is an exhaustive round-robin scan of all points in the pull path.
/// Every frame, its @ref m_currentIdx increments, so it will eventually check every point in the
/// pull path. If the kart is within the current segment, it calls @ref KartPullPath::changePoint to
/// update the current point and distance.
void KartPullPathTracker::calcTrackerGlobal() {
    s16 idx;
    EGG::Vector3f point, dir;
    if (search(SearchDirection::Current, idx, point, dir)) {
        m_handle->changePoint(idx, getDistance(point, dir));
    }

    if (static_cast<size_t>(++m_currentIdx) >= m_pointInfo->pointCount() - 1) {
        m_currentIdx = 0;
    }
}

/// @addr{0x80593814}
/// @brief Checks if the kart is within the regional tracker's current, next, or previous segment
/// @details The regional tracker checks the current segment, the next segment, and the previous
/// segment, skipping the other checks once it succeeds. If the kart is within any of those
/// segments, it calls @ref KartPullPath::changePoint to update the current point and distance.
void KartPullPathTracker::calcTrackerRegional() {
    if (m_handle->incomingIdx() < 0) {
        return;
    }

    s16 idx;
    EGG::Vector3f point, dir;

    if (search(SearchDirection::Current, idx, point, dir)) {
        m_handle->changePoint(idx, getDistance(point, dir));
        return;
    }

    m_handle->resetDistance();
    if (search(SearchDirection::Next, idx, point, dir)) {
        m_handle->changePoint(idx, getDistance(point, dir));
        return;
    }

    if (search(SearchDirection::Previous, idx, point, dir)) {
        m_handle->changePoint(idx, getDistance(point, dir));
    }
}

/// @addr{0x80593310}
/// @brief Gets the distance from the kart to the line formed by the point and direction.
/// @param point A point on the line
/// @param dir The direction of the line
/// @return The distance from the kart to the line formed by the point and direction
f32 KartPullPathTracker::getDistance(const EGG::Vector3f &point, const EGG::Vector3f &dir) const {
    EGG::Vector3f diff = pos() - point;
    f32 dist = diff.length();
    f32 x = EGG::Mathf::abs(diff.dot(dir));
    return EGG::Mathf::sqrt(dist * dist - x * x);
}

/// @addr{0x8059345C}
/// @brief Searches for the current point in the pull path based on the given search direction
/// @param searchDirection The direction to search for the current point in the pull path
/// @param idx The index of the current point in the pull path, if found
/// @param point The position of the current point in the pull path, if found
/// @return `true` if the kart lies within the searched segment, `false` otherwise
/// @pre Requires that @ref m_pointInfo was set via @ref setPointInfo().
bool KartPullPathTracker::search(SearchDirection searchDirection, s16 &idx, EGG::Vector3f &point,
        EGG::Vector3f &dir) const {
    ASSERT(m_pointInfo);

    s16 p, c, n;
    switch (searchDirection) {
    case SearchDirection::Current:
        idx = m_currentIdx;
        p = -1;
        c = 0;
        n = 1;
        break;
    case SearchDirection::Next:
        idx = m_currentIdx + 1;
        p = 0;
        c = 1;
        n = 2;
        break;
    case SearchDirection::Previous:
        idx = m_currentIdx - 1;
        p = -2;
        c = -1;
        n = 0;
        break;
    default:
        PANIC("Invalid search direction!");
        break;
    }

    const auto &points = m_pointInfo->points();

    // There always needs to be a current point and a next point to get the direction
    if (m_currentIdx + c < 0 || static_cast<size_t>(m_currentIdx + c) >= points.size()) {
        return false;
    }

    if (m_currentIdx + n < 0 || static_cast<size_t>(m_currentIdx + n) >= points.size()) {
        return false;
    }

    const auto &currentPos = points[m_currentIdx + c].pos;
    const auto &nextPos = points[m_currentIdx + n].pos;

    EGG::Vector3f back = currentPos - nextPos;
    back.normalise();
    EGG::Vector3f front = -back;

    if (m_currentIdx + p >= 0 && static_cast<size_t>(m_currentIdx + p) < points.size()) {
        const auto &prevPos = points[m_currentIdx + p].pos;
        back = prevPos - currentPos;
        back.normalise();
    }

    EGG::Plane3f backPlane = EGG::Plane3f(currentPos, back);
    EGG::Plane3f frontPlane = EGG::Plane3f(nextPos, front);

    if (backPlane.testPoint(pos()) && frontPlane.testPoint(pos())) {
        point = currentPos;
        dir = front;
        return true;
    }

    return false;
}

/// @addr{0x80593FA4}
/// @brief Constructor
/// @details Constructs @ref m_globalTracker and @ref m_regionalTracker and calls @ref init().
KartPullPath::KartPullPath()
    : m_globalTracker(this, KartPullPathTracker::Type::Global),
      m_regionalTracker(this, KartPullPathTracker::Type::Regional) {
    init();
}

/// @addr{0x80594094}
/// @brief Default virtual destructor
KartPullPath::~KartPullPath() = default;

/// @addr{0x805940D4}
/// @brief Initializes the pull path subsystem
/// @details Calls @reset(). Initializes @ref m_areaId to `-1` and @ref m_roadSpeedDecay to
/// `1.0f`.
void KartPullPath::init() {
    reset();
    m_areaId = -1;
    m_roadSpeedDecay = 1.0f;
}

/// @addr{0x80593CB8}
/// @brief Resets the pull path subsystem to its default state
/// @details Sets @ref m_distance to `-1.0f` to indicate that no valid distance has been found this
/// frame. Resets @ref m_currentIdx and @ref m_incomingIdx to `-1`. Resets @ref m_pointInfo to
/// `nullptr`, @ref m_pullDirection to `EGG::Vector3f::zero`, @ref m_pullSpeed to zero`, and @ref
/// m_maxPullSpeed to zero.
void KartPullPath::reset() {
    m_distance = -1.0f;
    m_currentIdx = -1;
    m_incomingIdx = -1;
    m_pointInfo = nullptr;
    m_pullDirection = EGG::Vector3f::zero;
    m_pullSpeed = 0.0f;
    m_maxPullSpeed = 0.0f;
}

/// @addr{0x80594134}
/// @brief Every frame, updates the @ref KartPullPathTracker objects to find the current segment
/// of the pull path
/// @details If there is no active pull path area, it bails out. Otherwise, as long as the pull
/// path area has more than 2 points, calls @ref calcTrackers() to update the trackers.
/// @pre If there is an active pull path area, requires that @ref m_pointInfo is valid.
void KartPullPath::calc() {
    if (calcArea()) {
        ASSERT(m_pointInfo);
        if (m_pointInfo->pointCount() > 2) {
            calcTrackers();
        }
    }
}

/// @addr{0x80593DBC}
/// @brief Changes the current point in the pull path to the given index and distance, if it's
/// closer than what's already been found this frame
/// @param idx The index of the point in the pull path to change to
/// @param distance The distance from the line formed by the point and direction
/// @details This is called whenever the trackers' search functions succeed. If the distance is
/// absurdly large or is further away than the current best found this frame, it bails out.
/// Otherwise, it updates @ref m_incomingIdx to the provided `idx`, @ref m_distance to the
/// provided `distance`, sets the provided `idx` in @ref m_regionalTracker, and calculates the
/// pull direction via @ref calcPointChange().
void KartPullPath::changePoint(s16 idx, f32 distance) {
    if ((m_distance >= 0.0f || distance >= 3000.0f) && distance >= m_distance) {
        return;
    }

    m_incomingIdx = idx;
    m_distance = distance;
    m_regionalTracker.setCurrentIdx(idx);
    calcPointChange();
}

/// @addr{0x80593E08}
/// @brief Clears the current distance, so that the next call to @ref changePoint will succeed
/// @details Sets @ref m_distance to `-1.0f` to indicate that no valid distance has been found
/// this frame.
void KartPullPath::resetDistance() {
    m_distance = -1.0f;
}

/// @brief Gets the index of the closest candidate segment found this frame
/// @return The index of the closest candidate segment found this frame, @ref m_incomingIdx
s16 KartPullPath::incomingIdx() const {
    return m_incomingIdx;
}

/// @brief Gets the current pull direction
/// @return The current pull direction, @ref m_pullDirection
const EGG::Vector3f &KartPullPath::pullDirection() const {
    return m_pullDirection;
}

/// @brief Gets the current pull speed
/// @return The current pull speed, @ref m_pullSpeed
f32 KartPullPath::pullSpeed() const {
    return m_pullSpeed;
}

/// @brief Gets the maximum pull speed
/// @return The maximum pull speed, @ref m_maxPullSpeed
f32 KartPullPath::maxPullSpeed() const {
    return m_maxPullSpeed;
}

/// @brief Gets the road speed decay factor
/// @return The road speed decay factor, @ref m_roadSpeedDecay
f32 KartPullPath::roadSpeedDecay() const {
    return m_roadSpeedDecay;
}

/// @addr{0x805941BC}
/// @brief Checks to see if the kart is within a pull path area and updates the current point info
/// @return `true` if the kart is within a pull path area, `false` otherwise
/// @details Checks if the kart is within a pull path area via @ref
/// System::CourseMap::getCurrentAreaID(). Based off of @ref System::MapdataAreaBase params, also
/// sets the speed decay factor and max pull speed.
bool KartPullPath::calcArea() {
    auto *courseMap = System::CourseMap::Instance();

    s16 prevAreaId = m_areaId;
    m_areaId = System::CourseMap::Instance()->getCurrentAreaID(m_areaId, pos(),
            System::MapdataAreaBase::Type::MovingRoad);

    if (m_areaId >= 0) {
        if (prevAreaId < 0 || prevAreaId != m_areaId) {
            reset();
            auto *area = courseMap->getArea(m_areaId);
            ASSERT(area);
            auto *pointInfo = area->getPointInfo();
            ASSERT(pointInfo);
            m_pointInfo = pointInfo;
            if (pointInfo->pointCount() > 2) {
                setTrackerPointInfo(pointInfo);
            } else {
                m_incomingIdx = 0;
                calcPointChange();
            }
            m_roadSpeedDecay = 0.9f + 0.001f * static_cast<f32>(area->param(0));
            m_maxPullSpeed = static_cast<f32>(area->param(1));
        }
    } else {
        m_areaId = -1;
        m_currentIdx = -1;
        m_incomingIdx = -1;
        m_pullDirection = EGG::Vector3f::zero;
    }

    return m_areaId >= 0;
}

/// @addr{0x80593E18}
/// @brief Updates the current point in the pull path to the incoming index and computes the pull
/// direction
/// @details The pull direction is first computed by simply finding the direction from the new
/// current segment position to the next segment position. Based off the current point's settings,
/// the pull direction may be forward, left, or right.
void KartPullPath::calcPointChange() {
    if (m_currentIdx == m_incomingIdx) {
        return;
    }

    const auto &points = m_pointInfo->points();

    const auto &currentPos = points[m_incomingIdx].pos;
    const auto &nextPos = points[m_incomingIdx + 1].pos;
    m_pullSpeed = static_cast<f32>(points[m_incomingIdx].setting[0]);
    u16 pullInfluence = points[m_incomingIdx + 1].setting[1];

    m_pullDirection = nextPos - currentPos;
    m_pullDirection.normalise();

    // If the pull influence is 0, the pull direction moves forward, so do nothing
    // If the pull influence is 1, the pull direction moves left
    if (pullInfluence == 1) {
        m_pullDirection = getPullSideDirection() * -1.0f;
    }

    // If the pull influence is 2, the pull direction moves right
    else if (pullInfluence == 2) {
        m_pullDirection = getPullSideDirection();
    }

    m_currentIdx = m_incomingIdx;
}

/// @addr{0x80593D54}
/// @brief Runs the global and regional trackers' per-frame calc functions
void KartPullPath::calcTrackers() {
    m_globalTracker.calc();
    m_regionalTracker.calc();
}

/// @addr{0x80593D1C}
/// @brief Sets the current point info for the trackers to search through
/// @param info The point info to set for the trackers
void KartPullPath::setTrackerPointInfo(const System::MapdataPointInfo *info) {
    m_globalTracker.setPointInfo(info);
    m_regionalTracker.setPointInfo(info);
}

/// @addr{0x805AEAD8}
/// @brief Computes the unit vector perpendicular to the pull direction in the XZ plane
/// @return The sideways direction of the pull direction
/// @details This is used to redirect the pull path sideways instead of forward. This isn't a
/// part of KartPullPath, but is only called from this class.
EGG::Vector3f KartPullPath::getPullSideDirection() const {
    // Dot product of two unit vectors being 1.0f => angle between them is 0
    // They're the same vector, just return zero
    if (EGG::Vector3f::ey.dot(m_pullDirection) == 1.0f) {
        return EGG::Vector3f::zero;
    }

    EGG::Vector3f nrm = EGG::Vector3f::ey.cross(m_pullDirection);
    nrm.normalise();
    return nrm;
}

} // namespace Kinoko::Kart
