#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

/// @brief Describes a respawn point in a course
class MapdataJugemPoint {
public:
    /// @brief Represents the raw data structure of a respawn point
    struct SData {
        EGG::Vector3f pos;   ///< Position of the respawn point
        EGG::Vector3f rot;   ///< Rotation of the respawn point
        u8 _18[0x1c - 0x18]; ///< Unused in Kinoko
    };
    static_assert(sizeof(SData) == 0x1C);

    /// @addr{0x805183A8}
    /// @brief Constructor
    /// @param data Pointer to the raw respawn point data
    MapdataJugemPoint(const SData *data) : m_rawData(data) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
        read(stream);
    }

    /// @brief Reads the respawn point data from the given stream
    /// @param stream The stream to read from
    void read(EGG::Stream &stream) {
        m_pos.read(stream);
        m_rot.read(stream);
    }

    /// @beginGetters
    const EGG::Vector3f &pos() const {
        return m_pos;
    }

    const EGG::Vector3f &rot() const {
        return m_rot;
    }
    /// @endGetters

private:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw respawn point data
    EGG::Vector3f m_pos;                           ///< Position of the respawn point
    EGG::Vector3f m_rot;                           ///< Rotation of the respawn point
};

/// @brief Provides access to entries in the JGPT section of the course KMP
class MapdataJugemPointAccessor
    : public MapdataAccessorBase<MapdataJugemPoint, MapdataJugemPoint::SData> {
public:
    /// @addr{Inlined at 0x805130C4}
    /// @brief Constructor
    /// @param header Pointer to the section header of the JGPT section
    MapdataJugemPointAccessor(const MapSectionHeader *header)
        : MapdataAccessorBase<MapdataJugemPoint, MapdataJugemPoint::SData>(header) {
        init(reinterpret_cast<const MapdataJugemPoint::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    }

    /// @brief Default virtual destructor
    ~MapdataJugemPointAccessor() override = default;
};

} // namespace Kinoko::System
