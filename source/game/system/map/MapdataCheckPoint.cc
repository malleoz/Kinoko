#include "MapdataCheckPoint.hh"

#include "game/system/CourseMap.hh"
#include "game/system/map/MapdataCheckPath.hh"

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

    // Performance improvement for Kinoko - a unique identifier representing the last search
    // performed, which avoids having to clear a "searched" flag for every checkpoint.
    m_lastSearchedId = 0;
}

/// @brief Default destructor
MapdataCheckPoint::~MapdataCheckPoint() = default;

/// @brief Reads the checkpoint data from the given stream
/// @param stream The stream to read from
void MapdataCheckPoint::read(EGG::RamStream &stream) {
    m_left.read(stream);
    m_right.read(stream);
    m_jugemIndex = stream.read_s8();
    m_type = stream.read_s8();
    m_prevId = stream.read_u8();
    m_nextId = stream.read_u8();
}

/// @addr{0x80515624}
/// @brief Initializes @ref m_nextPoints and @ref m_prevPoints from @ref m_nextId and @ref m_prevId
/// @details Also calculates the quadrilaterals for each of the next checkpoints in @ref
/// m_nextPoints.
void MapdataCheckPoint::initCheckpointLinks(MapdataCheckPointAccessor &accessor, int id) {
    m_id = id;
    const auto *checkPathAccessor = CourseMap::Instance()->checkPathAccessor();

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

/// @brief Finds at what fractional millisecond of a frame the player enters the checkpoint
/// @param prevPos The previous position, likely not located in the checkpoint.
/// @param pos The current position, likely located in the checkpoint.
/// @return The exact offset that crosses into the checkpoint, in the range [0, 1000 / 59.94].
/// @details This assumes the player is entering the checkpoint as intended, and not from the
/// side. This function isn't in the base game, but it can be used to determine improvements to
/// runs.
f32 MapdataCheckPoint::getEntryOffsetExact(const EGG::Vector2f &prevPos,
        const EGG::Vector2f &pos) const {
    constexpr f32 REFRESH_PERIOD = 1000.0f / 59.94f;

    EGG::Vector2f velocity = pos - prevPos;
    velocity *= 1.0f / REFRESH_PERIOD;

    // d_k = p_0 - m + kv
    // d_k dot r = 0 => k is the exact offset to the finish line
    // Therefore, k = ((m - p_0) dot r) / (v dot r)

    f32 x = (m_midpoint - prevPos).dot(m_dir);
    f32 y = velocity.dot(m_dir);

    // y = 0 => v is parallel to the checkpoint line
    return y != 0.0f ? x / y : 0.0f;
}

/// @brief Checks whether this checkpoint is a regular checkpoint
/// @return `true` if this checkpoint is a regular checkpoint (@ref Type::NormalCheckpoint),
/// `false` otherwise.
bool MapdataCheckPoint::isNormalCheckpoint() const {
    return static_cast<Type>(m_type) == Type::NormalCheckpoint;
}

/// @brief Checks whether this checkpoint increments the lap count
/// @return `true` if this checkpoint increments the lap count (@ref Type::FinishLine), `false`
/// otherwise.
bool MapdataCheckPoint::isFinishLine() const {
    return static_cast<Type>(m_type) == Type::FinishLine;
}

/// @brief Marks this checkpoint as searched in the DFS traversal this frame
/// @param epoch The current @ref CourseMap::m_sectorSearchId
/// @details Sets @ref m_lastSearchedId to `epoch`.
void MapdataCheckPoint::setSearched(u32 epoch) {
    m_lastSearchedId = epoch;
}

/// @brief Clears the searched status of this checkpoint
/// @details Resets @ref m_lastSearchedId so that it does not match any search epoch.
void MapdataCheckPoint::clearSearched() {
    m_lastSearchedId = 0;
}

/// @brief Checks whether this checkpoint has been searched in the DFS traversal this frame
/// @param epoch The current @ref CourseMap::m_sectorSearchId
/// @return `true` if this checkpoint has been searched, `false` otherwise.
bool MapdataCheckPoint::searched(u32 epoch) const {
    return m_lastSearchedId == epoch;
}

/// @brief Gets the index of the respawn point associated with this checkpoint
/// @return The respawn point index, @ref m_jugemIndex.
s8 MapdataCheckPoint::jugemIndex() const {
    return m_jugemIndex;
}

/// @brief Gets the type of this checkpoint
/// @return The checkpoint's type, @ref m_type.
s8 MapdataCheckPoint::type() const {
    return m_type;
}

/// @brief Gets the number of next checkpoints linked to this checkpoint
/// @return The number of next checkpoints, @ref m_nextCount.
u16 MapdataCheckPoint::nextCount() const {
    return m_nextCount;
}

/// @brief Gets the number of previous checkpoints linked to this checkpoint
/// @return The number of previous checkpoints, @ref m_prevCount.
u16 MapdataCheckPoint::prevCount() const {
    return m_prevCount;
}

/// @brief Gets the direction vector of this checkpoint
/// @return The direction vector, @ref m_dir.
const EGG::Vector2f &MapdataCheckPoint::dir() const {
    return m_dir;
}

/// @brief Gets the ID of this checkpoint
/// @return The checkpoint's ID, @ref m_id.
u16 MapdataCheckPoint::id() const {
    return m_id;
}

/// @brief Gets the previous checkpoint at the given index
/// @param i The index of the previous checkpoint
/// @return A pointer to the previous checkpoint
/// @pre Requires that `i` is less than the size of @ref m_prevPoints (aka @ref MAX_NEIGHBORS)
MapdataCheckPoint *MapdataCheckPoint::prevPoint(size_t i) const {
    ASSERT(i < m_prevPoints.size());
    return m_prevPoints[i];
}

/// @brief Gets the next checkpoint at the given index
/// @param i The index of the next checkpoint
/// @return A pointer to the next checkpoint
/// @pre Requires that `i` is less than the size of @ref m_nextPoints (aka @ref MAX_NEIGHBORS)
MapdataCheckPoint *MapdataCheckPoint::nextPoint(size_t i) const {
    ASSERT(i < m_nextPoints.size());
    return m_nextPoints[i].checkpoint;
}

/// @addr{0x80510C74}
/// @brief Classifies where the player's position sits relative to the quadrilateral formed
/// between this checkpoint and the next checkpoint
/// @param next The linked checkpoint to check against
/// @param p0 The XZ vector from the next checkpoint's left boundary to the player
/// @param p1 The XZ vector from this checkpoint right boundary to the player
/// @param distanceRatio The distance ratio reference to set
/// @return The sector occupancy of the player's position relative to the checkpoint quad
MapdataCheckPoint::SectorOccupancy MapdataCheckPoint::checkSectorAndDistanceRatio(
        const LinkedCheckpoint &next, const EGG::Vector2f &p0, const EGG::Vector2f &p1,
        f32 &distanceRatio) const {
    if (!checkSector(next, p0, p1)) {
        return SectorOccupancy::OutsideSector;
    }

    return checkDistanceRatio(next, p0, p1, distanceRatio) ? SectorOccupancy::InsideSector :
                                                             SectorOccupancy::BetweenSides;
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

/// @addr{0x80510BF0}
/// @brief Computes the the distance ratio: progress of traversal through the checkpoint
/// quad
/// @param next The linked checkpoint to check against
/// @param p0 The XZ vector from the next checkpoint's left boundary to the player
/// @param p1 The XZvector from this checkpoint right boundary to the player
/// @param distanceRatio The distance ratio reference to set
/// @return Whether the distance ratio is in its valid range, [0, 1].
bool MapdataCheckPoint::checkDistanceRatio(const LinkedCheckpoint &next, const EGG::Vector2f &p0,
        const EGG::Vector2f &p1, f32 &distanceRatio) const {
    f32 d1 = m_dir.dot(p1);
    f32 d2 = -(next.checkpoint->m_dir.dot(p0));
    distanceRatio = d1 / (d1 + d2);
    return distanceRatio >= 0.0f && distanceRatio <= 1.0f;
}

/// @brief Constructor
/// @param header Pointer to the map section header containing the checkpoint data
MapdataCheckPointAccessor::MapdataCheckPointAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataCheckPoint, MapdataCheckPoint::SData>(header) {
    MapdataAccessorBase::init(
            reinterpret_cast<const MapdataCheckPoint::SData *>(m_sectionHeader + 1),
            parse<u16>(m_sectionHeader->count));
    init();
}

/// @brief Default virtual destructor
MapdataCheckPointAccessor::~MapdataCheckPointAccessor() = default;

/// @beginGetters

/// @brief Gets the type of the last key checkpoint encountered
/// @return The type of the last key checkpoint encountered, @ref m_lastKcpType
s8 MapdataCheckPointAccessor::lastKcpType() const {
    return m_lastKcpType;
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
