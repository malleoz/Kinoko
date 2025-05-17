#include "ObjectKStick.hh"

#include "game/system/RaceConfig.hh"

namespace Field {

ObjectKStick::ObjectKStick(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    if (System::RaceConfig::Instance()->raceScenario().course == Course::Moonview_Highway) {
        registerManagedObject();
    }
}

ObjectKStick::~ObjectKStick() = default;

} // namespace Field
