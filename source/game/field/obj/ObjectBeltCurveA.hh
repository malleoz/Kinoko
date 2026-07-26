#pragma once

#include "game/field/obj/ObjectBelt.hh"

namespace Kinoko::Field {

/// @brief The set of curved conveyor belts at the end of the factory on Toad's Factory
/// @details This conveyor switches direction twice.
class ObjectBeltCurveA final : public ObjectBelt {
public:
    ObjectBeltCurveA(const System::MapdataGeoObj &params);
    ~ObjectBeltCurveA() override;

    [[nodiscard]] EGG::Vector3f calcRoadVelocity(u32 variant, const EGG::Vector3f &pos,
            u32 timeOffset) const override;

    /// @addr{0x807FCCA4}
    [[nodiscard]] bool isMoving(u32 variant, const EGG::Vector3f & /*pos*/) const override {
        return variant == 4 || variant == 5;
    }

private:
    [[nodiscard]] f32 calcDirSwitchVelocity(u32 t) const;
    [[nodiscard]] bool isMovingForward(u32 t) const;

    bool m_startForward;   ///< Whether the belts start moving forward or backward
    u16 m_dirChange1Frame; ///< Frame of first direction change
    u16 m_dirChange2Frame; ///< Frame of second direction change
    EGG::Matrix34f m_rotMat;
};

} // namespace Kinoko::Field
