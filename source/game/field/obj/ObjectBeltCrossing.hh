#pragma once

#include "game/field/obj/ObjectBelt.hh"

namespace Kinoko::Field {

/// @brief The conveyers in the first section of Toad's Factory.
/// @details Comprises of two conveyers that move in opposite directions with variant 0 moving in
/// the negative direction and variant 1 moving in the positive direction.
class ObjectBeltCrossing final : public ObjectBelt {
public:
    /// @addr{0x807FC764}
    ObjectBeltCrossing(const System::MapdataGeoObj &params) : ObjectBelt(params) {
        m_roadVel = 28.0f;
    }

    /// @addr{0x807FD8A8}
    ~ObjectBeltCrossing() override = default;

    /// @addr{0x807FC7D8}
    [[nodiscard]] EGG::Vector3f calcRoadVelocity(u32 variant, const EGG::Vector3f & /*pos*/,
            u32 /*timeOffset*/) const override {
        switch (variant) {
        case 0:
            return -EGG::Vector3f::ex * m_roadVel;
        case 1:
            return EGG::Vector3f::ex * m_roadVel;
        default:
            return EGG::Vector3f::zero;
        }
    }

    /// @addr{0x807FC874}
    [[nodiscard]] bool isMoving(u32 variant, const EGG::Vector3f & /*pos*/) const override {
        return variant == 0 || variant == 1;
    }
};

} // namespace Kinoko::Field
