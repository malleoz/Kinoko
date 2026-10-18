#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

namespace Kinoko {

namespace EGG {

class RamStream;

}

namespace System {

class MapdataCheckPathAccessor;

/// @brief Describes a checkpoint path in a course
class MapdataCheckPath {
public:
    /// @brief The number of neighboring checkpoint paths that each checkpoint path tracks
    static constexpr size_t MAX_NEIGHBORS = 6;

    /// @brief Raw data structure for a checkpoint path in a course
    struct SData {
        u8 start;               ///< Index of the first checkpoint in this checkpath
        u8 size;                ///< Number of checkpoints in this checkpath
        u8 prev[MAX_NEIGHBORS]; ///< Indices of the six previous checkpoint groups
        u8 next[MAX_NEIGHBORS]; ///< Indices of the six next checkpoint groups
        u8 _0e[0x10 - 0x0e];    ///< Padding
    };
    STATIC_ASSERT(sizeof(SData) == 0x10);

    MapdataCheckPath(const SData *data);
    ~MapdataCheckPath();

    void read(EGG::RamStream &stream);
    void findDepth(s8 depth, const MapdataCheckPathAccessor &accessor);

    [[nodiscard]] bool isPointInPath(u16 checkpointId) const;

    /// @beginGetters

    [[nodiscard]] u8 start() const;
    [[nodiscard]] u8 end() const;
    [[nodiscard]] const std::array<u8, MAX_NEIGHBORS> &next() const;
    [[nodiscard]] const std::array<u8, MAX_NEIGHBORS> &prev() const;
    [[nodiscard]] s8 depth() const;
    [[nodiscard]] f32 invCount() const;

    /// @endGetters

private:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw checkpoint path data
    u8 m_start;                           ///< Index of the first checkpoint in this checkpath
    u8 m_size;                            ///< Number of checkpoints in this checkpath
    std::array<u8, MAX_NEIGHBORS> m_prev; ///< Indices of previous connected checkpaths
    std::array<u8, MAX_NEIGHBORS> m_next; ///< Indices of next connected checkpaths
    s8 m_depth;                           ///< Number of checkpaths away from first checkpath
    f32 m_invCount; ///< Inverse of the number of checkpoints in this checkpath
};

/// @brief Provides access to entries in the `CKPH` section of the course KMP
class MapdataCheckPathAccessor
    : public MapdataAccessorBase<MapdataCheckPath, MapdataCheckPath::SData> {
public:
    MapdataCheckPathAccessor(const MapSectionHeader *header);
    ~MapdataCheckPathAccessor() override;

    [[nodiscard]] MapdataCheckPath *findCheckpathForCheckpoint(u16 checkpointId) const;

    /// @beginGetters

    [[nodiscard]] f32 lapProportion() const;

    /// @endGetters

private:
    /// @brief Minimum proportion of a lap a checkpath can be. Calculated as
    /// 1/(maxDepth+1).
    /// @details Another way to think of it: maxDepth+1 is the number of
    /// checkpaths in the longest route through the course, where longest means
    /// *most checkpaths traversed*, not most _distance_ traversed. So if one
    /// plans their route to hit the most checkpaths possible (no backtracking),
    /// they hit maxDepth+1 checkpaths, and each checkpath is `lapProportion`%
    /// of the total checkpaths on the route.
    f32 m_lapProportion;
};

} // namespace System

} // namespace Kinoko
