#include "MapdataCheckPoint.hh"

#include "game/system/CourseMap.hh"

#include <ranges>

namespace Kinoko::System {

/// @addr{0x805154E4}
/// @brief Constructor
/// @param data Pointer to the raw checkpoint data
/// @details Parses the raw checkpoint data from the provided `SData` pointer. Afterwards, it
/// calculates @ref m_midpoint based on @ref m_left and @ref m_right. Finally, calculates @ref m_dir
/// based on the direction from @ref m_left to @ref m_right.
MapdataCheckPoint::MapdataCheckPoint(const SData *data)
    : m_rawData(data),
      m_nextCount(0),
      m_prevCount(0) {
    EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
    read(stream);
    m_midpoint = 0.5f * (m_left + m_right);
    m_dir = EGG::Vector2f(m_right.y - m_left.y, m_left.x - m_right.x);
    m_dir.normalise();
}

/// @addr{0x80515624}
/// @brief Initializes @ref m_nextPoints and @ref m_prevPoints from @ref m_nextId and @ref m_prevId
/// @details Also calculates the quadrilaterals for each of the next checkpoints in @ref
/// m_nextPoints.
void MapdataCheckPoint::initCheckpointLinks(MapdataCheckPointAccessor &accessor, int id) {
    m_id = id;
    const auto *checkPathAccessor = CourseMap::Instance()->checkPath();

    // Calculate the quadrilateral's @ref m_prevPoints. If the check point is the first in its
    // group, it has multiple previous checkpoints defined by its preceding checkpaths
    if (m_prevId == 0xFF) {
        MapdataCheckPath *checkpath = checkPathAccessor->findCheckpathForCheckpoint(id);
        if (checkpath) {
            m_prevCount = 0;

            for (auto [i, prevID] : std::views::enumerate(checkpath->prev())) {
                if (prevID == 0xFF) {
                    continue;
                }

                m_prevPoints[i] = accessor.get(checkPathAccessor->get(prevID)->end());
                ++m_prevCount;
            }
        }
    } else {
        m_prevPoints[0] = accessor.get(m_prevId);
        ++m_prevCount;
    }

    // Calculate the quadrilateral's @ref m_nextPoints. If the checkpoint is the last in its group,
    // it can have multiple quadrilaterals (and nextCheckpoint) which are determined by its next
    // path(s)
    if (m_nextId == 0xFF) {
        MapdataCheckPath *checkpath = checkPathAccessor->findCheckpathForCheckpoint(id);
        if (checkpath) {
            m_nextCount = 0;

            for (auto [i, nextID] : std::views::enumerate(checkpath->next())) {
                if (nextID == 0xFF) {
                    continue;
                }

                m_nextPoints[i].checkpoint = accessor.get(checkPathAccessor->get(nextID)->start());
                ++m_nextCount;
            }
        }
    } else {
        m_nextPoints[0].checkpoint = accessor.get(m_nextId);
        ++m_nextCount;
    }

    // Form the checkpoint's quadrilateral(s)
    for (auto [i, next] : std::views::enumerate(m_nextPoints)) {
        if (i < m_nextCount) {
            auto &nextLinked = m_nextPoints[i];
            auto *nextPoint = nextLinked.checkpoint;

            next.distance = (nextPoint->m_midpoint - m_midpoint).normalise();
            next.p0diff = nextPoint->m_left - m_left;
            next.p1diff = nextPoint->m_right - m_right;
        } else {
            next.distance = 0.0f;
            next.p0diff = EGG::Vector2f::zero;
            next.p1diff = EGG::Vector2f::zero;
        }
    }
}

/// @addr{0x80510D7C}
/// @brief Checks which sector the given position is in relative to the checkpoint and computes the
/// distance ratio
/// @param pos The position to check
/// @param distanceRatio Reference to store the computed distance ratio
/// @return The sector occupancy status of the position
/// @details Iterates each checkpoint in @ref m_nextPoints and calls @ref
/// checkSectorAndDistanceRatio() for each. If it returned @ref SectorOccupancy::InsideSector, the
/// iteration stops and that value is returned. Otherwise, if it returned @ref
/// SectorOccupancy::BetweenSides, the iteration continues and that status is noted. If no
/// checkpoint returns @ref SectorOccupancy::InsideSector, the final return value will reflect
/// whether any checkpoint was between sides or all were outside.
MapdataCheckPoint::SectorOccupancy MapdataCheckPoint::checkSectorAndDistanceRatio(
        const EGG::Vector3f &pos, f32 &distanceRatio) const {
    bool betweenSides = false;
    EGG::Vector2f p1 = m_right;
    p1.y = pos.z - p1.y;
    p1.x = pos.x - p1.x;

    for (size_t i = 0; i < m_nextCount; ++i) {
        const LinkedCheckpoint &next = m_nextPoints[i];
        EGG::Vector2f p0 = next.checkpoint->m_left;
        p0.y = pos.z - p0.y;
        p0.x = pos.x - p0.x;
        MapdataCheckPoint::SectorOccupancy result =
                checkSectorAndDistanceRatio(next, p0, p1, distanceRatio);

        if (result == SectorOccupancy::InsideSector) {
            return SectorOccupancy::InsideSector;
        } else if (result == SectorOccupancy::BetweenSides) {
            betweenSides = true;
        }
    }

    return betweenSides ? SectorOccupancy::BetweenSides : SectorOccupancy::OutsideSector;
}

/// @addr{0x80511EC8}
/// @brief Finds at what millisecond subdivision of a frame the player enters the checkpoint
/// @param prevPos The previous position, likely not located in the checkpoint.
/// @param pos The current position, likely located in the checkpoint.
/// @return The earliest subdivision that crosses into the checkpoint, in the range [1, 17].
/// @details Computes the player's displacement per millisecond based on the provided `pos` and
/// `prevPos`. Computes the relative position between the checkpoint's @ref m_midpoint and `prevPos`
/// advanced by one millisecond. Iterates one millisecond at a time adding the per-millisecond
/// displacement until the player crosses the checkpoint.
/// @note This assumes the player is entering the checkpoint as intended, and not from the side.
u16 MapdataCheckPoint::getEntryOffsetMs(const EGG::Vector2f &prevPos,
        const EGG::Vector2f &pos) const {
    constexpr f32 REFRESH_PERIOD = 1000.0f / 59.94f;

    EGG::Vector2f velocity = pos - prevPos;
    velocity *= 1.0f / REFRESH_PERIOD;

    // d_k = p_0 - m + kv
    // We walk along the line via k increments, until we pass it
    // The goal of this function is to approximate k as an integer
    EGG::Vector2f displacement = prevPos - m_midpoint + velocity;

    u16 k = 1;
    for (; static_cast<f32>(k) < REFRESH_PERIOD && displacement.dot(m_dir) < 0.0f; ++k) {
        displacement += velocity;
    }

    return k;
}

/// @addr{0x0x80510B84}
/// @brief Checks whether the player is between the two sides of the checkpoint quad
/// @param next The linked checkpoint to check against
/// @param p0 The XZ vector from the next checkpoint's left boundary to the player
/// @param p1 The XZ vector from this checkpoint right boundary to the player
/// @return `true` if the player is between the two sides of the checkpoint quad, `false` otherwise
/// @details Uses cross products to validate that the player's position sits between the two sides
/// of the checkpoint quad.
bool MapdataCheckPoint::checkSector(const MapdataCheckPoint::LinkedCheckpoint &next,
        const EGG::Vector2f &p0, const EGG::Vector2f &p1) const {
    if (-(next.p0diff.y) * p0.x + next.p0diff.x * p0.y < 0.0f) {
        return false;
    }

    if (next.p1diff.y * p1.x - next.p1diff.x * p1.y < 0.0f) {
        return false;
    }

    return true;
}

/// @addr{0x80515244}
/// @brief Initializes all checkpoint links, and finds the finish line and last key checkpoint
/// @details Initializes each checkpoint's links. If the checkpoint is the finish line, its ID is
/// saved to @ref m_finishLineCheckpointId. The type of the last key checkpoint encountered is saved
/// to @ref m_lastKcpType.
void MapdataCheckPointAccessor::init() {
    s8 lastKcpType = -1;
    s16 finishLineCheckpointId = -1;

    for (size_t ckptId = 0; ckptId < size(); ckptId++) {
        MapdataCheckPoint *checkpoint = get(ckptId);
        checkpoint->initCheckpointLinks(*this, ckptId);

        if (checkpoint->isFinishLine()) {
            finishLineCheckpointId = ckptId;
        }

        lastKcpType = std::max(lastKcpType, checkpoint->type());
    }

    m_lastKcpType = lastKcpType;
    m_finishLineCheckpointId = finishLineCheckpointId;
}

} // namespace Kinoko::System
