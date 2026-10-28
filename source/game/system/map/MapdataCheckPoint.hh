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
    ~MapdataCheckPoint();

    void read(EGG::RamStream &stream);
    void initCheckpointLinks(MapdataCheckPointAccessor &accessor, int id);

    [[nodiscard]] SectorOccupancy checkSectorAndDistanceRatio(const EGG::Vector3f &pos,
            f32 &distanceRatio) const;
    [[nodiscard]] u16 getEntryOffsetMs(const EGG::Vector2f &prevPos,
            const EGG::Vector2f &pos) const;

    [[nodiscard]] f32 getEntryOffsetExact(const EGG::Vector2f &prevPos,
            const EGG::Vector2f &pos) const;

    [[nodiscard]] bool isNormalCheckpoint() const;
    [[nodiscard]] bool isFinishLine() const;

    /// @beginSetters

    void setSearched(u32 epoch);
    void clearSearched();

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] bool searched(u32 epoch) const;
    [[nodiscard]] s8 jugemIndex() const;
    [[nodiscard]] s8 type() const;
    [[nodiscard]] u16 nextCount() const;
    [[nodiscard]] u16 prevCount() const;
    [[nodiscard]] const EGG::Vector2f &dir() const;
    [[nodiscard]] u16 id() const;
    [[nodiscard]] MapdataCheckPoint *prevPoint(size_t i) const;
    [[nodiscard]] MapdataCheckPoint *nextPoint(size_t i) const;

    /// @endGetters

    /// @brief Describes the type of checkpoint
    /// @details Key checkpoints can take on values between 1 and 126. The type value represents
    /// the index, meaning players must pass key checkpoint 1, then 2, then 3, etc.
    enum class Type {
        NormalCheckpoint = -1, ///< Used to calculate respawns
        FinishLine = 0,        ///< Triggers a lap change
    };

private:
    [[nodiscard]] SectorOccupancy checkSectorAndDistanceRatio(const LinkedCheckpoint &next,
            const EGG::Vector2f &p0, const EGG::Vector2f &p1, f32 &distanceRatio) const;
    [[nodiscard]] bool checkSector(const LinkedCheckpoint &next, const EGG::Vector2f &p0,
            const EGG::Vector2f &p1) const;
    [[nodiscard]] bool checkDistanceRatio(const LinkedCheckpoint &next, const EGG::Vector2f &p0,
            const EGG::Vector2f &p1, f32 &distanceRatio) const;

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
    u16 m_id;                 ///< ID of the checkpoint

    /// @brief The @ref CourseMap::m_sectorSearchId in which this checkpoint was last searched
    /// @details This is a performance improvement for Kinoko. The base game stores a boolean that
    /// is cleared on every checkpoint before each search in @ref CourseMap::findSector(). By
    /// referencing @ref m_lastSearchedId instead, we can determine if the checkpoint has been
    /// found in the current search without having to clear it explicitly.
    u32 m_lastSearchedId;

    /// @brief Array of previous checkpoint pointers (only @ref m_prevCount are valid)
    std::array<MapdataCheckPoint *, MAX_NEIGHBORS> m_prevPoints;

    /// @brief Array of next checkpoint links (only @ref m_nextCount are valid)
    std::array<LinkedCheckpoint, MAX_NEIGHBORS> m_nextPoints;
};

/// @brief Provides access to entries in the `CKPT` section of the course KMP
class MapdataCheckPointAccessor
    : public MapdataAccessorBase<MapdataCheckPoint, MapdataCheckPoint::SData> {
public:
    MapdataCheckPointAccessor(const MapSectionHeader *header);
    ~MapdataCheckPointAccessor() override;

    /// @beginGetters

    [[nodiscard]] s8 lastKcpType() const;

    /// @endGetters

private:
    void init();

    s8 m_lastKcpType;             ///< The type of the last key checkpoint encountered
    u16 m_finishLineCheckpointId; ///< The ID of the finish line checkpoint
};

} // namespace Kinoko::System
