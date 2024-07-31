#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace System {

/// @brief This class represents the course's respawn positions.
class MapdataJugemPoint {
public:
    struct SData {
        EGG::Vector3f pos;
        EGG::Vector3f rot;
        s16 padding;
        s16 _1a;
    };
    static_assert(sizeof(SData) == 0x1C);

    MapdataJugemPoint(const SData *data);
    void read(EGG::Stream &stream);

    /// @beginGetters
    const EGG::Vector3f &pos() const;
    const EGG::Vector3f &rot() const;
    /// @endGetters

private:
    const SData *m_rawData;
    EGG::Vector3f m_pos;
    EGG::Vector3f m_rot;
};

class MapdataJugemPointAccessor
    : public MapdataAccessorBase<MapdataJugemPoint, MapdataJugemPoint::SData> {
public:
    MapdataJugemPointAccessor(const MapSectionHeader *header);
    ~MapdataJugemPointAccessor() override;
};

} // namespace System
