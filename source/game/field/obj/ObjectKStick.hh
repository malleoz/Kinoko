#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectKStick : public ObjectCollidable {
public:
    ObjectKStick(const System::MapdataGeoObj &params);
    ~ObjectKStick();

private:
    ;
};

} // namespace Field
