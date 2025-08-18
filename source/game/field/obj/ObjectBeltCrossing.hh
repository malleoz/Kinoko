#pragma once

#include "game/field/obj/ObjectBelt.hh"

namespace Field {

class ObjectBeltCrossing final : public ObjectBelt {
public:
    ObjectBeltCrossing(const System::MapdataGeoObj &params);
    ~ObjectBeltCrossing() override;

    [[nodiscard]] EGG::Vector3f calcRoadVelocity(u32 variant, const EGG::Vector3f &pos,
            u32 timeOffset) const override;

    [[nodiscard]] bool isMoving(u32 variant, const EGG::Vector3f &pos) const override;
};

} // namespace Field
