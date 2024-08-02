#pragma once

#include "game/system/map/MapdataJugemPoint.hh"

namespace System {

class RaceMode {
public:
    RaceMode();
    ~RaceMode();

    const MapdataJugemPoint *jugemPoint();
};

} // namespace System
