#include "MapdataPointInfo.hh"

namespace Kinoko::System {

/// @brief Constructor
/// @param data Pointer to the raw route data
/// @details Creates a @ref EGG::RamStream from the raw point data and reads the data into the
/// member variables.
MapdataPointInfo::MapdataPointInfo(const SData *data) : m_rawData(data) {
    EGG::RamStream stream =
            EGG::RamStream(data, sizeof(SData) + parse<u16>(data->pointCount) * sizeof(Point));
    read(stream);
}

/// @brief Default destructor
MapdataPointInfo::~MapdataPointInfo() = default;

/// @brief Reads the route information data from the given stream
/// @param stream The stream to read from
void MapdataPointInfo::read(EGG::RamStream &stream) {
    u16 count = stream.read_u16();

    m_points = owning_span<Point>(count);

    for (auto &setting : m_settings) {
        setting = stream.read_u8();
    }

    for (auto &point : m_points) {
        EGG::Vector3f pos;
        pos.read(stream);

        u16 settings[2];
        settings[0] = stream.read_u16();
        settings[1] = stream.read_u16();

        point = Point(pos, {settings[0], settings[1]});
    }
}

/// @brief Gets the number of points in the route
/// @return The number of points in the route, aka the size of  @ref m_points.
size_t MapdataPointInfo::pointCount() const {
    return m_points.size();
}

/// @brief Gets the route setting at the specified index
/// @param idx The index of the route setting to retrieve
/// @return The route setting at the specified index, @ref m_settings.
/// @pre `idx` must be less than the number of route settings (2).
u8 MapdataPointInfo::setting(size_t idx) const {
    ASSERT(idx < m_settings.size());
    return m_settings[idx];
}

/// @brief Gets the array of points in the route
/// @return The array of points in the route, @ref m_points.
const owning_span<MapdataPointInfo::Point> &MapdataPointInfo::points() const {
    return m_points;
}

/// @addr{0x80515D3C}
/// @brief Constructor
/// @param header Pointer to the section header of the POTI section
MapdataPointInfoAccessor::MapdataPointInfoAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataPointInfo, MapdataPointInfo::SData>(header) {
    init(reinterpret_cast<const MapdataPointInfo::SData *>(m_sectionHeader + 1),
            parse<u16>(m_sectionHeader->count));
}

/// @brief Default virtual destructor
MapdataPointInfoAccessor::~MapdataPointInfoAccessor() = default;

/// @copydoc MapdataAccessorBase::init()
/// @details Creates @ref MapdataPointInfo instances for each entry in the `POTI` section.
void MapdataPointInfoAccessor::init(const MapdataPointInfo::SData *start, u16 count) {
    if (count != 0) {
        m_entries.reserve(count);
    }

    uintptr_t data = reinterpret_cast<uintptr_t>(start);

    for (u16 i = 0; i < count; ++i) {
        m_entries.push_back(
                EGG::egg_new<MapdataPointInfo>(reinterpret_cast<MapdataPointInfo::SData *>(data)));
        data += m_entries[i]->pointCount() * sizeof(MapdataPointInfo::Point) +
                offsetof(MapdataPointInfo::SData, points);
    }
}

} // namespace Kinoko::System
