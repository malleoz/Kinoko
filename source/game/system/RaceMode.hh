#pragma once

#include "game/system/map/MapdataJugemPoint.hh"

namespace System {

class RaceManager;

class RaceMode {
public:
    RaceMode(RaceManager *raceManager);
    ~RaceMode();

    const MapdataJugemPoint *jugemPoint();

private:
    const RaceManager *m_raceManager;
};

} // namespace System
