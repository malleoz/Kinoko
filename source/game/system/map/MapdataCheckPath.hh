#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/util/Stream.hh>

namespace Kinoko::System {

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

    /// @addr{0x80515098}
    /// @brief Constructor
    /// @param data Pointer to the raw checkpoint path data
    /// @details Creates a @ref EGG::RamStream from the raw cannon point data and reads the data
    /// into the member variables. Also sets @ref m_invCount to `1.0f / @ref m_size`.
    MapdataCheckPath(const SData *data) : m_rawData(data), m_depth(-1) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
        read(stream);
        m_invCount = 1.0f / m_size;
    }

    /// @brief Default destructor
    ~MapdataCheckPath() = default;

    void read(EGG::Stream &stream);

    void findDepth(s8 depth, const MapdataCheckPathAccessor &accessor);

    /// @brief Checks if the provided checkpoint id lies within this checkpoint path
    /// @param checkpointId The checkpoint id to check
    /// @return `true` if the checkpoint id lies within this checkpoint path, `false` otherwise
    [[nodiscard]] bool isPointInPath(u16 checkpointId) const {
        return m_start <= checkpointId && checkpointId <= end();
    }

    /// @beginGetters

    /// @brief Gets the index of the first checkpoint in this checkpath
    /// @return The index of the first checkpoint in this checkpath
    [[nodiscard]] u8 start() const {
        return m_start;
    }

    /// @brief Gets the index of the last checkpoint in this checkpath
    /// @return The index of the last checkpoint in this checkpath
    [[nodiscard]] u8 end() const {
        return m_start + m_size - 1;
    }

    /// @brief Gets the indices of the next connected checkpoint paths
    /// @return The indices of the next connected checkpoint paths
    [[nodiscard]] const std::array<u8, MAX_NEIGHBORS> &next() const {
        return m_next;
    }

    /// @brief Gets the indices of the previous connected checkpoint paths
    /// @return The indices of the previous connected checkpoint paths
    [[nodiscard]] const std::array<u8, MAX_NEIGHBORS> &prev() const {
        return m_prev;
    }

    /// @brief Gets the depth of this checkpoint path in the traversal order
    /// @return The depth of this checkpoint path
    [[nodiscard]] s8 depth() const {
        return m_depth;
    }

    /// @brief Gets the inverse of the number of checkpoints in this checkpath
    /// @return The inverse of the number of checkpoints in this checkpath
    [[nodiscard]] f32 invCount() const {
        return m_invCount;
    }

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

    /// @brief Default virtual destructor
    ~MapdataCheckPathAccessor() override = default;

    /// @addr{0x80515014}
    /// @brief Finds the checkpoint path that contains the specified checkpoint id
    /// @param checkpointId The checkpoint id to search for
    /// @return Pointer to the checkpoint path containing the checkpoint id, or `nullptr` if not
    /// found.
    [[nodiscard]] MapdataCheckPath *findCheckpathForCheckpoint(u16 checkpointId) const {
        for (auto &checkpath : m_entries) {
            if (checkpath->isPointInPath(checkpointId)) {
                return checkpath;
            }
        }

        return nullptr;
    }

    /// @beginGetters

    /// @brief Gets the minimum proportion of a lap a checkpath can be
    /// @return The minimum proportion of a lap a checkpath can be
    [[nodiscard]] f32 lapProportion() const {
        return m_lapProportion;
    }

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

} // namespace Kinoko::System
