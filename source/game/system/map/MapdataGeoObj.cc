#include "MapdataGeoObj.hh"

namespace Kinoko::System {

/// @brief Constructor
/// @param data Pointer to the raw KMP object data
/// @details Creates a @ref EGG::RamStream from the raw object data and reads the data into the
/// member variables.
MapdataGeoObj::MapdataGeoObj(const SData *data) : m_rawData(data) {
    EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
    read(stream);
}

/// @brief Default destructor
MapdataGeoObj::~MapdataGeoObj() = default;

/// @brief Reads the object data from the given stream
/// @param stream The stream to read from
void MapdataGeoObj::read(EGG::RamStream &stream) {
    m_id = stream.read_u16();
    stream.skip(2);
    m_pos.read(stream);
    m_rot.read(stream);
    m_scale.read(stream);
    m_pathId = stream.read_s16();

    for (auto &setting : m_settings) {
        setting = stream.read_u16();
    }

    m_presenceFlag = stream.read_u16();
}

/// @brief Gets the ID of the object
/// @return The ID of the object
u16 MapdataGeoObj::id() const {
    return m_id;
}

/// @brief Gets the position of the object
/// @return The position of the object
const EGG::Vector3f &MapdataGeoObj::pos() const {
    return m_pos;
}

/// @brief Gets the rotation of the object
/// @return The rotation of the object
const EGG::Vector3f &MapdataGeoObj::rot() const {
    return m_rot;
}

/// @brief Gets the scale of the object
/// @return The scale of the object
const EGG::Vector3f &MapdataGeoObj::scale() const {
    return m_scale;
}

/// @brief Gets the path ID of the object
/// @return The path ID of the object
s16 MapdataGeoObj::pathId() const {
    return m_pathId;
}

/// @brief Gets the setting of the object at the specified index
/// @param idx The index of the setting to retrieve
/// @return The setting of the object at the specified index
/// @pre `idx` must be less than the number of settings (8).
u16 MapdataGeoObj::setting(size_t idx) const {
    ASSERT(idx < m_settings.size());
    return m_settings[idx];
}

/// @brief Gets the presence flag of the object
/// @return The presence flag of the object
u16 MapdataGeoObj::presenceFlag() const {
    return m_presenceFlag;
}

/// @brief Constructor
/// @param header Pointer to the section header of the GOBJ section
MapdataGeoObjAccessor::MapdataGeoObjAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataGeoObj, MapdataGeoObj::SData>(header) {
    init(reinterpret_cast<const MapdataGeoObj::SData *>(m_sectionHeader + 1),
            parse<u16>(m_sectionHeader->count));
}

/// @brief Default virtual destructor
MapdataGeoObjAccessor::~MapdataGeoObjAccessor() = default;

} // namespace Kinoko::System
