#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

/// @brief This class represents the course's respawn positions.
class MapdataJugemPoint {
public:
    struct SData {
        EGG::Vector3f pos;
        EGG::Vector3f rot;
        u8 _18[0x1c - 0x18];
    };
    static_assert(sizeof(SData) == 0x1c);

    /// @addr{0x805183A8}
    MapdataJugemPoint(const SData *data) : m_rawData(data) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
        read(stream);
    }

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
    [[maybe_unused]] const SData *m_rawData;
    EGG::Vector3f m_pos;
    EGG::Vector3f m_rot;
};

class MapdataJugemPointAccessor
    : public MapdataAccessorBase<MapdataJugemPoint, MapdataJugemPoint::SData> {
public:
    /// @addr{Inlined at 0x805130C4}
    MapdataJugemPointAccessor(const MapSectionHeader *header)
        : MapdataAccessorBase<MapdataJugemPoint, MapdataJugemPoint::SData>(header) {
        init(reinterpret_cast<const MapdataJugemPoint::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    }

    ~MapdataJugemPointAccessor() override = default;
};

} // namespace Kinoko::System
