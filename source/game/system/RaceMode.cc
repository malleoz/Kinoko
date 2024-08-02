#include "RaceMode.hh"

#include "game/system/CourseMap.hh"
namespace System {

RaceMode::RaceMode() = default;

RaceMode::~RaceMode() = default;

/// @addr{0x80535EF4}
const MapdataJugemPoint *RaceMode::jugemPoint() {
    constexpr u16 TEMP_RSL_JUGEMPOINT_IDX = 17;

    auto *courseMap = CourseMap::Instance();

    if (TEMP_RSL_JUGEMPOINT_IDX >= courseMap->getJugemPointCount()) {
        return courseMap->getJugemPoint(0);
    }

    return CourseMap::Instance()->getJugemPoint(TEMP_RSL_JUGEMPOINT_IDX);
}

} // namespace System
