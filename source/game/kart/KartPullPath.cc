#include "KartPullPath.hh"

#include "game/system/CourseMap.hh"

#include <egg/geom/Plane.hh>

namespace Kinoko::Kart {

/// @addr{0x8059308C}
/// @brief Constructor
/// @param handle Pointer to the owning @ref KartPullPath subsystem
/// @param type The type of tracker to use for the pull path
KartPullPathTracker::KartPullPathTracker(KartPullPath *handle, Type type)
    : m_type(type),
      m_currentIdx(0),
      m_pointInfo(nullptr),
      m_handle(handle) {}

/// @addr{0x805930E4}
/// @brief Default destructor
KartPullPathTracker::~KartPullPathTracker() = default;

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

/// @addr{0x8059345C}
/// @brief Searches for the current point in the pull path based on the given search direction
/// @param searchDirection The direction to search for the current point in the pull path
/// @param idx The index of the current point in the pull path, if found
/// @param point The position of the current point in the pull path, if found
/// @return True if the kart lies within the searched segment, false otherwise
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
/// @brief Constructor which initializes the global and regional pull path trackers and initializes
/// the pull path
KartPullPath::KartPullPath()
    : m_globalTracker(this, KartPullPathTracker::Type::Global),
      m_regionalTracker(this, KartPullPathTracker::Type::Regional) {
    init();
}

/// @addr{0x80594094}
/// @brief Default destructor
KartPullPath::~KartPullPath() = default;

/// @addr{0x80593CB8}
/// @brief Resets the pull path subsystem to its default state
/// @details This was the init function in the base class, but it gets inlined in calcArea.
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
/// @brief Every frame, updates the @ref KartPullPathTracker objects to find the current segment of
/// the pull path
/// @details If there is no active pull path area, it bails out.
void KartPullPath::calc() {
    if (!calcArea()) {
        return;
    }

    ASSERT(m_pointInfo);
    if (m_pointInfo->pointCount() > 2) {
        calcTrackers();
    }
}

/// @addr{0x805941BC}
/// @brief Checks to see if the kart is within a pull path area and updates the current point info
/// @return True if the kart is within a pull path area, false otherwise
/// @details Based off of @ref System::MapdataAreaBase params, also sets the speed decay factor and
/// max pull speed.
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

} // namespace Kinoko::Kart
