#include "game/system/map/MapdataCheckPath.hh"

namespace System {

/// @addr{0x80515098}
MapdataCheckPath::MapdataCheckPath(const SData *data) : m_rawData(data), m_depth(-1) {
    u8 *unsafeData = reinterpret_cast<u8 *>(const_cast<SData *>(data));
    EGG::RamStream stream = EGG::RamStream(unsafeData, sizeof(SData));
    read(stream);
    m_oneOverCount = 1.0f / m_size;
}

void MapdataCheckPath::read(EGG::Stream &stream) {
    m_start = stream.read_u8();
    m_size = stream.read_u8();
    for (auto &prev : m_prev) {
        prev = stream.read_u8();
    }

    for (auto &next : m_next) {
        next = stream.read_u8();
    }
}

/// @brief performs DFS to calculate `m_depth` (distance (in checkpaths) from the first checkpath)
/// for all subsequent checkpaths. if that doesnt make sense just paste this text into chatgpt and
/// ask it to explain it to you.
/// @param depth number of checkpaths from first checkpath
/// @addr{0x805150E0}
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

// u16 MapdataCheckPath::getPrev(u16 i) const {
//     return m_prev[i];
// }

// u16 MapdataCheckPath::getNext(u16 i) const {
//     return m_next[i];
// }

u8 MapdataCheckPath::start() const {
    return m_start;
}

// u8 MapdataCheckPath::size() const {
//     return m_size;
// }

u8 MapdataCheckPath::end() const {
    return m_start + m_size - 1;
}

const std::array<u8, MapdataCheckPath::MAX_NEIGHBORS> &MapdataCheckPath::next() const {
    return m_next;
}

const std::array<u8, MapdataCheckPath::MAX_NEIGHBORS> &MapdataCheckPath::prev() const {
    return m_prev;
}

s8 MapdataCheckPath::depth() const {
    return m_depth;
}

f32 MapdataCheckPath::oneOverCount() const {
    return m_oneOverCount;
}

bool MapdataCheckPath::isPointInPath(u16 checkpointId) const {
    return start() <= checkpointId && checkpointId <= end();
}

f32 MapdataCheckPathAccessor::lapProportion() {
    return m_lapProportion;
}

/// @addr{0x80515014}
MapdataCheckPath *MapdataCheckPathAccessor::findCheckpathForCheckpoint(u16 checkpointId) const {
    for (size_t i = 0; i < size(); ++i) {
        MapdataCheckPath *checkpath = get(i);
        if (checkpath->isPointInPath(checkpointId)) {
            return checkpath;
        }
    }

    return nullptr;
}

/// @addr{Inlined in 0x8051377C}
MapdataCheckPathAccessor::MapdataCheckPathAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataCheckPath, MapdataCheckPath::SData>(header) {
    init(reinterpret_cast<const MapdataCheckPath::SData *>(m_sectionHeader + 1),
            parse<u16>(m_sectionHeader->count));
    // maximum number of checkpaths one could traverse through in a lap
    s8 maxDepth = -1;
    get(0)->findDepth(0, *this);

    for (size_t i = 0; i < size(); i++) {
        maxDepth = std::max(maxDepth, get(i)->depth());
    }

    m_lapProportion = 1.0f / (maxDepth + 1.0f);
    // loadPaths();
}

MapdataCheckPathAccessor::~MapdataCheckPathAccessor() = default;

} // namespace System
