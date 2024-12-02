#include "RaceMode.hh"

#include "game/system/CourseMap.hh"
#include "game/system/RaceManager.hh"

namespace System {

RaceMode::RaceMode(RaceManager *raceManager) : m_raceManager(raceManager) {};

RaceMode::~RaceMode() = default;

/// @addr{0x80535EF4}
const MapdataJugemPoint *RaceMode::jugemPoint() {
    return CourseMap::Instance()->getJugemPoint(m_raceManager->player().respawn());
}

} // namespace System
