#pragma once

#include "game/field/obj/ObjectBelt.hh"

namespace Kinoko::Field {

/// @brief The conveyers outside of the factory on Toad's Factory.
class ObjectBeltEasy final : public ObjectBelt {
public:
    ObjectBeltEasy(const System::MapdataGeoObj &params);
    ~ObjectBeltEasy() override;

private:
    [[nodiscard]] EGG::Vector3f calcRoadVelocity(u32 variant, const EGG::Vector3f &pos,
            u32 timeOffset) const override;
    [[nodiscard]] bool isMoving(u32 variant, const EGG::Vector3f &pos) const override;
};

} // namespace Kinoko::Field
