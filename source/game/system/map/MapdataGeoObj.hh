#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

/// @brief Describes a KMP object in a course
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

    /// @brief Constructor
    /// @param data Pointer to the raw KMP object data
    MapdataGeoObj(const SData *data) : m_rawData(data) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
        read(stream);
    }

    /// @brief Reads the object data from the given stream
    /// @param stream The stream to read from
    void read(EGG::Stream &stream) {
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

    /// @beginGetters
    [[nodiscard]] u16 id() const {
        return m_id;
    }

    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    [[nodiscard]] const EGG::Vector3f &rot() const {
        return m_rot;
    }

    [[nodiscard]] const EGG::Vector3f &scale() const {
        return m_scale;
    }

    [[nodiscard]] s16 pathId() const {
        return m_pathId;
    }

    [[nodiscard]] u16 setting(size_t idx) const {
        ASSERT(idx < m_settings.size());
        return m_settings[idx];
    }

    [[nodiscard]] u16 presenceFlag() const {
        return m_presenceFlag;
    }
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

/// @brief Provides access to entries in the GOBJ section of the course KMP
class MapdataGeoObjAccessor : public MapdataAccessorBase<MapdataGeoObj, MapdataGeoObj::SData> {
public:
    /// @brief Constructor
    /// @param header Pointer to the section header of the GOBJ section
    MapdataGeoObjAccessor(const MapSectionHeader *header)
        : MapdataAccessorBase<MapdataGeoObj, MapdataGeoObj::SData>(header) {
        init(reinterpret_cast<const MapdataGeoObj::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    }

    /// @brief Default virtual destructor
    ~MapdataGeoObjAccessor() override = default;
};

} // namespace Kinoko::System
