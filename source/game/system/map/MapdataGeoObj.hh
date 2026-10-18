#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

/// @brief Describes a `KMP object` in a course
class MapdataGeoObj {
public:
    /// @brief Represents the raw data structure of a KMP object
    struct SData {
        u16 id;                 ///< The @ref Field::ObjectId of the object
        EGG::Vector3f position; ///< The position of the object
        EGG::Vector3f rotation; ///< The rotation of the object
        EGG::Vector3f scale;    ///< The scale of the object
        s16 pathId;             ///< The @ref Field::Rail ID of the object, if any
        u16 settings[8];        ///< The settings of the object
        u16 presenceFlag; ///< Controls whether the object is loaded depending on number of players
    };

    MapdataGeoObj(const SData *data);
    ~MapdataGeoObj();

    void read(EGG::RamStream &stream);

    /// @beginGetters

    [[nodiscard]] u16 id() const;
    [[nodiscard]] const EGG::Vector3f &pos() const;
    [[nodiscard]] const EGG::Vector3f &rot() const;
    [[nodiscard]] const EGG::Vector3f &scale() const;
    [[nodiscard]] s16 pathId() const;
    [[nodiscard]] u16 setting(size_t idx) const;
    [[nodiscard]] u16 presenceFlag() const;

    /// @endGetters

private:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw KMP object data
    u16 m_id;                                      ///< The @ref Field::ObjectId of the object
    EGG::Vector3f m_pos;                           ///< The position of the object
    EGG::Vector3f m_rot;                           ///< The rotation of the object
    EGG::Vector3f m_scale;                         ///< The scale of the object
    s16 m_pathId;                  ///< The @ref Field::Rail ID of the object, if any
    std::array<u16, 8> m_settings; ///< The settings of the object
    u16 m_presenceFlag; ///< Controls whether the object is loaded depending on number of players
};

/// @brief Provides access to entries in the `GOBJ` section of the course KMP
class MapdataGeoObjAccessor : public MapdataAccessorBase<MapdataGeoObj, MapdataGeoObj::SData> {
public:
    MapdataGeoObjAccessor(const MapSectionHeader *header);
    ~MapdataGeoObjAccessor() override;
};

} // namespace Kinoko::System
