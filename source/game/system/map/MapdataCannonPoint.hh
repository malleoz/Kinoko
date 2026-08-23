#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include "game/kart/KartMove.hh"

namespace Kinoko::System {

class MapdataCannonPoint {
public:
    struct SData {
        EGG::Vector3f pos;
        EGG::Vector3f rot;
        u16 id;
        s16 propertyIdx;
    };

    MapdataCannonPoint(const SData *data) : m_rawData(data) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
        read(stream);
    }

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
    [[maybe_unused]] const SData *m_rawData;
    EGG::Vector3f m_pos;
    EGG::Vector3f m_rot;
    u16 m_id;
    s16 m_parameterIdx; ///< Index into the table of cannon properties. Used to determine speed,
                        ///< height, and decel
};

class MapdataCannonPointAccessor
    : public MapdataAccessorBase<MapdataCannonPoint, MapdataCannonPoint::SData> {
public:
    /// @addr{Inlined at 0x80512FA4}
    MapdataCannonPointAccessor(const MapSectionHeader *header)
        : MapdataAccessorBase<MapdataCannonPoint, MapdataCannonPoint::SData>(header) {
        init(reinterpret_cast<const MapdataCannonPoint::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    }

    ~MapdataCannonPointAccessor() override = default;
};

} // namespace Kinoko::System
