#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include "game/kart/KartMove.hh"

namespace Kinoko::System {

/// @brief Describes a cannon's target point in the course
class MapdataCannonPoint {
public:
    /// @brief Raw data structure for a cannon point in a course
    struct SData {
        EGG::Vector3f pos; ///< Position of the cannon target
        EGG::Vector3f rot; ///< Rotation of karts in this cannon
        u16 id;            ///< Effectively the index of the cannon point
        s16 parameterIdx;  ///< Index into the table of cannon properties
    };
    STATIC_ASSERT(sizeof(SData) == 0x1C);

    /// @brief Constructor
    /// @param data Pointer to the raw cannon point data
    MapdataCannonPoint(const SData *data) : m_rawData(data) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
        read(stream);
    }

    /// @brief Default destructor
    ~MapdataCannonPoint() = default;

    /// @brief Parses the raw cannon point data from the provided stream
    /// @param stream The stream from which to read the raw cannon point data
    void read(EGG::Stream &stream) {
        m_pos.read(stream);
        m_rot.read(stream);
        m_id = stream.read_u16();
        m_parameterIdx = stream.read_s16();
    }

    /// @beginGetters
    const EGG::Vector3f &pos() const {
        return m_pos;
    }

    const EGG::Vector3f &rot() const {
        return m_rot;
    }

    u16 id() const {
        return m_id;
    }

    s16 parameterIdx() const {
        return m_parameterIdx;
    }
    /// @endGetters

private:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw cannon point data
    EGG::Vector3f m_pos;                           ///< Position of the cannon target
    EGG::Vector3f m_rot;                           ///< Rotation of karts in this cannon
    u16 m_id;                                      ///< Effectively the index of the cannon point
    s16 m_parameterIdx;                            ///< Index into the table of cannon properties
};

/// @brief Provides access to entries in the CNPT section of the course KMP
class MapdataCannonPointAccessor
    : public MapdataAccessorBase<MapdataCannonPoint, MapdataCannonPoint::SData> {
public:
    /// @addr{Inlined at 0x80512FA4}
    /// @brief Constructor
    /// @param header Pointer to the header of the CNPT section
    MapdataCannonPointAccessor(const MapSectionHeader *header)
        : MapdataAccessorBase<MapdataCannonPoint, MapdataCannonPoint::SData>(header) {
        init(reinterpret_cast<const MapdataCannonPoint::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    }

    /// @brief Default virtual destructor
    ~MapdataCannonPointAccessor() override = default;
};

} // namespace Kinoko::System
