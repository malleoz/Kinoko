#include "MapdataCheckPoint.hh"

#include "game/system/CourseMap.hh"

#include <ranges>

namespace Kinoko::System {

/// @addr{0x805154E4}
MapdataCheckPoint::MapdataCheckPoint(const SData *data)
    : m_rawData(data), m_nextCount(0), m_prevCount(0) {
    EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
    read(stream);
    m_midpoint = 0.5f * (m_left + m_right);
    m_dir = EGG::Vector2f(m_right.y - m_left.y, m_left.x - m_right.x);
    m_dir.normalise();
}

/// @addr{0x80515624}
/// @brief Calculates @ref MapdataCheckPoint::m_nextPoints and @ref MapdataCheckPoint::m_prevPoints
/// from @ref MapdataCheckPoint::m_nextPt and @ref MapdataCheckPoint::m_prevPt.
/// @details Also calculates the quadrilaterals for the next checkpoints, filling the fields of @ref
/// LinkedCheckpoint for each.
void MapdataCheckPoint::initCheckpointLinks(MapdataCheckPointAccessor &accessor, int id) {
    m_id = id;
    const auto *checkPathAccessor = CourseMap::Instance()->checkPath();

    // Calculate the quadrilateral's `m_prevPoints`. If the check point is the first in its group,
    // it has multiple previous checkpoints defined by its preceding checkpaths
    if (m_prevPt == 0xFF) {
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
        m_prevPoints[0] = accessor.get(m_prevPt);
        ++m_prevCount;
    }

    // Calculate the quadrilateral's `m_nextPoints`. If the checkpoint is the last in its group, it
    // can have multiple quadrilaterals (and nextCheckpoint) which are determined by its next
    // path(s)
    if (m_nextPt == 0xFF) {
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
        m_nextPoints[0].checkpoint = accessor.get(m_nextPt);
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
/// @brief Finds the offset between the two positions that enter the checkpoint.
/// @details This assumes the player is entering the checkpoint as intended, and not from the side.
/// @param prevPos The previous position, likely not located in the checkpoint.
/// @param pos The current position, likely located in the checkpoint.
/// @return The earliest subdivision that crosses into the checkpoint, in the range [1, 17].
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
/// @return Whether the player is between the two sides of the checkpoint quad.
bool MapdataCheckPoint::checkSector(const LinkedCheckpoint &next, const EGG::Vector2f &p0,
        const EGG::Vector2f &p1) const {
    if (-(next.p0diff.y) * p0.x + next.p0diff.x * p0.y < 0.0f) {
        return false;
    }

    if (next.p1diff.y * p1.x - next.p1diff.x * p1.y < 0.0f) {
        return false;
    }

    return true;
}

MapdataCheckPointAccessor::MapdataCheckPointAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataCheckPoint, MapdataCheckPoint::SData>(header) {
    MapdataAccessorBase::init(
            reinterpret_cast<const MapdataCheckPoint::SData *>(m_sectionHeader + 1),
            parse<u16>(m_sectionHeader->count));
    init();
}

MapdataCheckPointAccessor::~MapdataCheckPointAccessor() = default;

/// @addr{0x80515244}
/// @brief Initializes all checkpoint links, and finds the finish line and last key checkpoint.
void MapdataCheckPointAccessor::init() {
    s8 lastKcpType = -1;
    s16 finishLineCheckpointId = -1;

    for (size_t ckptId = 0; ckptId < size(); ckptId++) {
        MapdataCheckPoint *checkpoint = get(ckptId);
        checkpoint->initCheckpointLinks(*this, ckptId);

        if (checkpoint->isFinishLine()) {
            finishLineCheckpointId = ckptId;
        }

        lastKcpType = std::max(lastKcpType, checkpoint->checkArea());
    }

    m_lastKcpType = lastKcpType;
    m_finishLineCheckpointId = finishLineCheckpointId;
}

} // namespace Kinoko::System
