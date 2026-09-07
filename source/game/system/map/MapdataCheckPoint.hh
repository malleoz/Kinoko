#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

class MapdataCheckPoint;
class MapdataCheckPointAccessor;

/// @brief Describes a checkpoint in a course
class MapdataCheckPoint {
public:
    /// @brief Raw data structure representing a checkpoint in the course
    struct SData {
        EGG::Vector2f left;  ///< Position of the left boundary of the checkpoint
        EGG::Vector2f right; ///< Position of the right boundary of the checkpoint
        s8 jugemIndex;       ///< The respawn point associated with this checkpoint
        s8 checkArea;        ///< The type of checkpoint area (normal checkpoint / finish line)
        u8 prevPt;           ///< ID of the previous checkpoint
        u8 nextPt;           ///< ID of the next checkpoint
    };
    STATIC_ASSERT(sizeof(SData) == 0x14);

    /// @brief Describes the player's position relative to the checkpoint area
    enum class SectorOccupancy {
        InsideSector,  ///< Player is inside the given checkpoint group
        OutsideSector, ///< Player is outside the given checkpoint group
        BetweenSides,  ///< Player is between sides of the checkpoint group but not
                       ///< between this checkpoint and next
    };

    /// @brief Bundles the next checkpoint pointer with data describing the quadrilateral formed
    /// between _this_ checkpoint and that next checkpoint
    struct LinkedCheckpoint {
        MapdataCheckPoint *checkpoint; ///< Pointer to the next checkpoint
        EGG::Vector2f p0diff;          ///< Diff between this checkpoint's and next's left boundary
        EGG::Vector2f p1diff;          ///< Diff between this checkpoint's and next's right boundary
        f32 distance; ///< Distance between the midpoints of this checkpoint and the next
    };

    MapdataCheckPoint(const SData *data);

    /// @brief Default destructor
    ~MapdataCheckPoint() = default;

    /// @brief Reads the checkpoint data from the given stream
    /// @param stream The stream to read from
    void read(EGG::Stream &stream) {
        m_left.read(stream);
        m_right.read(stream);
        m_jugemIndex = stream.read_s8();
        m_type = stream.read_s8();
        m_prevId = stream.read_u8();
        m_nextId = stream.read_u8();
    }

    void initCheckpointLinks(MapdataCheckPointAccessor &accessor, int id);
    [[nodiscard]] SectorOccupancy checkSectorAndDistanceRatio(const EGG::Vector3f &pos,
            f32 &distanceRatio) const;

    [[nodiscard]] u16 getEntryOffsetMs(const EGG::Vector2f &prevPos,
            const EGG::Vector2f &pos) const;

    /// @brief Finds at what fractional millisecond of a frame the player enters the checkpoint
    /// @param prevPos The previous position, likely not located in the checkpoint.
    /// @param pos The current position, likely located in the checkpoint.
    /// @return The exact offset that crosses into the checkpoint, in the range [0, 1000 / 59.94].
    /// @details This assumes the player is entering the checkpoint as intended, and not from the
    /// side. This function isn't in the base game, but it can be used to determine improvements to
    /// runs.
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

    /// @brief Checks whether this checkpoint is a regular checkpoint
    /// @return True if this checkpoint is a regular checkpoint, false otherwise.
    [[nodiscard]] bool isNormalCheckpoint() const {
        return static_cast<Type>(m_type) == Type::NormalCheckpoint;
    }

    /// @brief Checks whether this checkpoint increments the lap count
    /// @return True if this checkpoint increments the lap count, false otherwise.
    [[nodiscard]] bool isFinishLine() const {
        return static_cast<Type>(m_type) == Type::FinishLine;
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

    [[nodiscard]] s8 type() const {
        return m_type;
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

    /// @brief Describes the type of checkpoint
    /// @details Key checkpoints can take on values between 1 and 126. The type value represents the
    /// index, meaning players must pass key checkpoint 1, then 2, then 3, etc.
    enum class Type {
        NormalCheckpoint = -1, ///< Used to calculate respawns
        FinishLine = 0,        ///< Triggers a lap change
    };

private:
    /// @addr{0x80510C74}
    /// @brief Classifies where the player's position sits relative to the quadrilateral formed
    /// between this checkpoint and the next checkpoint
    /// @param next The linked checkpoint to check against
    /// @param p0 The XZ vector from the next checkpoint's left boundary to the player
    /// @param p1 The XZ vector from this checkpoint right boundary to the player
    /// @param distanceRatio The distance ratio reference to set
    /// @return The sector occupancy of the player's position relative to the checkpoint quad
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
    /// @brief Computes the the distance ratio: progress of traversal through the checkpoint quad
    /// @param next The linked checkpoint to check against
    /// @param p0 The XZ vector from the next checkpoint's left boundary to the player
    /// @param p1 The XZvector from this checkpoint right boundary to the player
    /// @param distanceRatio The distance ratio reference to set
    /// @return Whether the distance ratio is in its valid range, [0, 1].
    [[nodiscard]] bool checkDistanceRatio(const LinkedCheckpoint &next, const EGG::Vector2f &p0,
            const EGG::Vector2f &p1, f32 &distanceRatio) const {
        f32 d1 = m_dir.dot(p1);
        f32 d2 = -(next.checkpoint->m_dir.dot(p0));
        distanceRatio = d1 / (d1 + d2);
        return distanceRatio >= 0.0f && distanceRatio <= 1.0f;
    }

    static constexpr size_t MAX_NEIGHBORS = 6;

    [[maybe_unused]] const SData *m_rawData; ///< Pointer to the raw checkpoint data
    EGG::Vector2f m_left;     ///< The left boundary of the checkpoint in XZ coordinates
    EGG::Vector2f m_right;    ///< The right boundary of the checkpoint in XZ coordinates
    s8 m_jugemIndex;          ///< Index of respawn point associated with this checkpoint
    s8 m_type;                ///< The type of checkpoint (see @ref Type)
    u8 m_prevId;              ///< ID of the previous checkpoint
    u8 m_nextId;              ///< ID of the next checkpoint
    u16 m_nextCount;          ///< Number of next checkpoints
    u16 m_prevCount;          ///< Number of previous checkpoints
    EGG::Vector2f m_midpoint; ///< The midpoint of the checkpoint in XZ coordinates
    EGG::Vector2f m_dir;      ///< The direction vector of the checkpoint in XZ coordinates
    bool m_searched; ///< Indicates whether the checkpoint has been searched against this frame
    u16 m_id;        ///< ID of the checkpoint

    /// @brief Array of previous checkpoint pointers (only @ref m_prevCount are valid)
    std::array<MapdataCheckPoint *, MAX_NEIGHBORS> m_prevPoints;

    /// @brief Array of next checkpoint links (only @ref m_nextCount are valid)
    std::array<LinkedCheckpoint, MAX_NEIGHBORS> m_nextPoints;
};

/// @brief Provides access to entries in the CKPT section of the course KMP
class MapdataCheckPointAccessor
    : public MapdataAccessorBase<MapdataCheckPoint, MapdataCheckPoint::SData> {
public:
    MapdataCheckPointAccessor(const MapSectionHeader *header);
    ~MapdataCheckPointAccessor() override;

    /// @beginGetters
    [[nodiscard]] s8 lastKcpType() const {
        return m_lastKcpType;
    }
    /// @endGetters

private:
    void init();

    s8 m_lastKcpType;             ///< The type of the last key checkpoint encountered
    u16 m_finishLineCheckpointId; ///< The ID of the finish line checkpoint
};

} // namespace Kinoko::System
