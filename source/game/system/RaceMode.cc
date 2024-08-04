#include "RaceMode.hh"

#include "game/system/CourseMap.hh"
#include "game/system/RaceConfig.hh"

namespace System {

RaceMode::RaceMode() = default;

RaceMode::~RaceMode() = default;

/// @addr{0x80535EF4}
const MapdataJugemPoint *RaceMode::jugemPoint() {
    constexpr u16 TEMP_RSL_JUGEMPOINT_IDX = 17;
    constexpr u16 TEMP_RSGB_JUGEMPOINT_IDX = 8;
    constexpr u16 TEMP_RWS_JUGEMPOINT_IDX = 5;

    auto *raceCfg = System::RaceConfig::Instance();

    u16 idx;

    switch (raceCfg->raceScenario().course) {
    case Course::N64_Sherbet_Land:
        idx = TEMP_RSL_JUGEMPOINT_IDX;
        break;
    case Course::GBA_Shy_Guy_Beach:
        idx = TEMP_RSGB_JUGEMPOINT_IDX;
        break;
    case Course::GCN_Waluigi_Stadium:
        idx = TEMP_RWS_JUGEMPOINT_IDX;
        break;
    default:
        idx = 0;
        break;
    }

    return CourseMap::Instance()->getJugemPoint(idx);
}

} // namespace System
