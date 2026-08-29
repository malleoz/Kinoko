#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

class MapdataCheckPoint;
class MapdataCheckPointAccessor;

struct LinkedCheckpoint {
    MapdataCheckPoint *checkpoint;
    EGG::Vector2f p0diff;
    EGG::Vector2f p1diff;
    f32 distance;
};

class MapdataCheckPoint {
public:
    struct SData {
        EGG::Vector2f left;
        EGG::Vector2f right;
        s8 jugemIndex;
        s8 checkArea;
        u8 prevPt;
        u8 nextPt;
    };
    STATIC_ASSERT(sizeof(SData) == 0x14);

    enum class SectorOccupancy {
        InsideSector,  ///< Player is inside the given checkpoint group
        OutsideSector, ///< Player is outside the given checkpoint group
        BetweenSides,  ///< Player is between sides of the checkpoint group but not
                       ///< between this checkpoint and next
    };

    MapdataCheckPoint(const SData *data);

    void read(EGG::Stream &stream) {
        m_left.read(stream);
        m_right.read(stream);
        m_jugemIndex = stream.read_s8();
        m_checkArea = stream.read_s8();
        m_prevPt = stream.read_u8();
        m_nextPt = stream.read_u8();
    }

    void initCheckpointLinks(MapdataCheckPointAccessor &accessor, int id);
    [[nodiscard]] SectorOccupancy checkSectorAndDistanceRatio(const EGG::Vector3f &pos,
            f32 &distanceRatio) const;

    [[nodiscard]] u16 getEntryOffsetMs(const EGG::Vector2f &prevPos,
            const EGG::Vector2f &pos) const;

    /// @brief Finds the offset between the two positions that enter the checkpoint.
    /// @details This assumes the player is entering the checkpoint as intended, and not from the
    /// side. This function isn't in the base game, but it can be used to determine improvements to
    /// runs.
    /// @param prevPos The previous position, likely not located in the checkpoint.
    /// @param pos The current position, likely located in the checkpoint.
    /// @return The exact offset that crosses into the checkpoint, in the range [0, 1000 / 59.94].
    [[nodiscard]] f32 getEntryOffsetExact(const EGG::Vector2f &prevPos,
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

    [[nodiscard]] bool isNormalCheckpoint() const {
        return static_cast<CheckArea>(m_checkArea) == CheckArea::NormalCheckpoint;
    }

    [[nodiscard]] bool isFinishLine() const {
        return static_cast<CheckArea>(m_checkArea) == CheckArea::FinishLine;
    }

    /// @beginSetters
    void setSearched() {
        m_searched = true;
    }

    void clearSearched() {
        m_searched = false;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] bool searched() const {
        return m_searched;
    }

    [[nodiscard]] s8 jugemIndex() const {
        return m_jugemIndex;
    }

    [[nodiscard]] s8 checkArea() const {
        return m_checkArea;
    }

    [[nodiscard]] u16 nextCount() const {
        return m_nextCount;
    }

    [[nodiscard]] u16 prevCount() const {
        return m_prevCount;
    }

    [[nodiscard]] const EGG::Vector2f &dir() const {
        return m_dir;
    }

    [[nodiscard]] u16 id() const {
        return m_id;
    }

    [[nodiscard]] MapdataCheckPoint *prevPoint(size_t i) const {
        ASSERT(i < m_prevPoints.size());
        return m_prevPoints[i];
    }

    [[nodiscard]] MapdataCheckPoint *nextPoint(size_t i) const {
        ASSERT(i < m_nextPoints.size());
        return m_nextPoints[i].checkpoint;
    }
    /// @endGetters

    enum class CheckArea {
        NormalCheckpoint = -1, ///< Only used for picking respawn position
        FinishLine = 0,        ///< Triggers a lap change
    };

private:
    /// @addr{0x80510C74}
    [[nodiscard]] SectorOccupancy checkSectorAndDistanceRatio(const LinkedCheckpoint &next,
            const EGG::Vector2f &p0, const EGG::Vector2f &p1, f32 &distanceRatio) const {
        if (!checkSector(next, p0, p1)) {
            return SectorOccupancy::OutsideSector;
        }

        return checkDistanceRatio(next, p0, p1, distanceRatio) ? SectorOccupancy::InsideSector :
                                                                 SectorOccupancy::BetweenSides;
    }

    [[nodiscard]] bool checkSector(const LinkedCheckpoint &next, const EGG::Vector2f &p0,
            const EGG::Vector2f &p1) const;

    /// @addr{0x80510BF0}
    /// @brief Sets the distance ratio, which is the progress of traversal through the checkpoint
    /// quad.
    /// @param distanceRatio The distance ratio reference to set.
    /// @return Whether the distance ratio is in its valid range, [0, 1].
    [[nodiscard]] bool checkDistanceRatio(const LinkedCheckpoint &next, const EGG::Vector2f &p0,
            const EGG::Vector2f &p1, f32 &distanceRatio) const {
        f32 d1 = m_dir.dot(p1);
        f32 d2 = -(next.checkpoint->m_dir.dot(p0));
        distanceRatio = d1 / (d1 + d2);
        return distanceRatio >= 0.0f && distanceRatio <= 1.0f;
    }

    static constexpr size_t MAX_NEIGHBORS = 6;

    [[maybe_unused]] const SData *m_rawData;
    EGG::Vector2f m_left;
    EGG::Vector2f m_right;
    s8 m_jugemIndex; ///< Index of respawn point associated with this checkpoint. Players who die
                     ///< here will be respawned at this point.
    /// Either:
    /// - a @ref `NORMAL_CHECKPOINT` (-1) used to calculate respawns,
    /// - a @ref `FINISH_LINE` (0) which updates the lap count when crossed, or
    /// - a "key checkpoint" (1-127) used to ensure racers travel around the entire
    /// course before proceeding to the next lap. the type value represents the index,
    /// i.e. racers must pass checkpoint with @ref `m_type` 1, then 2, then 3 etc..
    s8 m_checkArea;
    u8 m_prevPt;
    u8 m_nextPt;
    u16 m_nextCount;
    u16 m_prevCount;
    EGG::Vector2f m_midpoint;
    EGG::Vector2f m_dir;
    bool m_searched;
    u16 m_id;
    std::array<MapdataCheckPoint *, MAX_NEIGHBORS> m_prevPoints;
    std::array<LinkedCheckpoint, MAX_NEIGHBORS> m_nextPoints;
};

class MapdataCheckPointAccessor
    : public MapdataAccessorBase<MapdataCheckPoint, MapdataCheckPoint::SData> {
public:
    MapdataCheckPointAccessor(const MapSectionHeader *header);
    ~MapdataCheckPointAccessor() override;

    [[nodiscard]] s8 lastKcpType() const {
        return m_lastKcpType;
    }

private:
    void init();

    s8 m_lastKcpType;
    u16 m_finishLineCheckpointId;
};

} // namespace Kinoko::System
