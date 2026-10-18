#include "game/system/map/MapdataCheckPath.hh"

#include <egg/util/Stream.hh>

namespace Kinoko::System {

/// @addr{0x80515098}
/// @brief Constructor
/// @param data Pointer to the raw checkpoint path data
/// @details Creates a @ref EGG::RamStream from the raw cannon point data and reads the data
/// into the member variables. Also sets @ref m_invCount to `1.0f / @ref m_size`.
MapdataCheckPath::MapdataCheckPath(const SData *data) : m_rawData(data), m_depth(-1) {
    EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
    read(stream);
    m_invCount = 1.0f / m_size;
}

/// @brief Default destructor
MapdataCheckPath::~MapdataCheckPath() = default;

/// @brief Reads the checkpoint path data from the given stream
/// @param stream The stream to read from
void MapdataCheckPath::read(EGG::RamStream &stream) {
    m_start = stream.read_u8();
    m_size = stream.read_u8();
    for (auto &prev : m_prev) {
        prev = stream.read_u8();
    }

    for (auto &next : m_next) {
        next = stream.read_u8();
    }
}

/// @addr{0x805150E0}
/// @brief Performs DFS to calculate @ref m_depth for all subsequent checkpaths.
/// @param depth Number of checkpaths from first checkpath.
/// @details Sets @ref m_depth to the provided `depth`. Then recursively calls `findDepth` on all
/// next connected checkpoint paths.
void MapdataCheckPath::findDepth(s8 depth, const MapdataCheckPathAccessor &accessor) {
    if (m_depth != -1) {
        return;
    }

    m_depth = depth;

    for (auto &nextID : m_next) {
        if (nextID == 0xFF) {
            continue;
        }

        accessor.get(nextID)->findDepth(depth + 1, accessor);
    }
}

/// @brief Checks if the provided checkpoint id lies within this checkpoint path
/// @param checkpointId The checkpoint id to check
/// @return `true` if the checkpoint id lies within this checkpoint path, `false` otherwise
bool MapdataCheckPath::isPointInPath(u16 checkpointId) const {
    return m_start <= checkpointId && checkpointId <= end();
}

/// @brief Gets the index of the first checkpoint in this checkpath
/// @return The index of the first checkpoint in this checkpath
u8 MapdataCheckPath::start() const {
    return m_start;
}

/// @brief Gets the index of the last checkpoint in this checkpath
/// @return The index of the last checkpoint in this checkpath
u8 MapdataCheckPath::end() const {
    return m_start + m_size - 1;
}

/// @brief Gets the indices of the next connected checkpoint paths
/// @return The indices of the next connected checkpoint paths
const std::array<u8, MapdataCheckPath::MAX_NEIGHBORS> &MapdataCheckPath::next() const {
    return m_next;
}

/// @brief Gets the indices of the previous connected checkpoint paths
/// @return The indices of the previous connected checkpoint paths
const std::array<u8, MapdataCheckPath::MAX_NEIGHBORS> &MapdataCheckPath::prev() const {
    return m_prev;
}

/// @brief Gets the depth of this checkpoint path in the traversal order
/// @return The depth of this checkpoint path
s8 MapdataCheckPath::depth() const {
    return m_depth;
}

/// @brief Gets the inverse of the number of checkpoints in this checkpath
/// @return The inverse of the number of checkpoints in this checkpath
f32 MapdataCheckPath::invCount() const {
    return m_invCount;
}

/// @addr{Inlined in 0x8051377C}
/// @brief Constructor
/// @param header Pointer to the map section header containing the checkpoint path data
/// @details Finds the maximum depth of the checkpoint paths and calculates the minimum lap
/// proportion accordingly.
MapdataCheckPathAccessor::MapdataCheckPathAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataCheckPath, MapdataCheckPath::SData>(header) {
    init(reinterpret_cast<const MapdataCheckPath::SData *>(m_sectionHeader + 1),
            parse<u16>(m_sectionHeader->count));

    if (empty()) {
        return;
    }

    // Maximum number of paths one could traverse through in a lap
    s8 maxDepth = -1;
    get(0)->findDepth(0, *this);

    for (size_t i = 0; i < size(); ++i) {
        maxDepth = std::max(maxDepth, get(i)->depth());
    }

    m_lapProportion = 1.0f / (maxDepth + 1.0f);
}

/// @brief Default virtual destructor
MapdataCheckPathAccessor::~MapdataCheckPathAccessor() = default;

/// @addr{0x80515014}
/// @brief Finds the checkpoint path that contains the specified checkpoint id
/// @param checkpointId The checkpoint id to search for
/// @return Pointer to the checkpoint path containing the checkpoint id, or `nullptr` if not
/// found.
MapdataCheckPath *MapdataCheckPathAccessor::findCheckpathForCheckpoint(u16 checkpointId) const {
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
f32 MapdataCheckPathAccessor::lapProportion() const {
    return m_lapProportion;
}

} // namespace Kinoko::System
