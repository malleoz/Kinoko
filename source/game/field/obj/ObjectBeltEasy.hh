#pragma once

#include "game/field/obj/ObjectBelt.hh"

namespace Kinoko::Field {

/// @brief The conveyers outside of the factory on Toad's Factory.
class ObjectBeltEasy final : public ObjectBelt {
public:
    /// @addr{0x807FC578}
    ObjectBeltEasy(const System::MapdataGeoObj &params) : ObjectBelt(params) {
        m_roadVel = 20.0f;
    }

    /// @addr{0x807FD8F0}
    ~ObjectBeltEasy() override = default;

private:
    /// @addr{0x807FC62C}
    [[nodiscard]] EGG::Vector3f calcRoadVelocity(u32 variant, const EGG::Vector3f & /*pos*/,
            u32 /*timeOffset*/) const override {
        switch (variant) {
        case 2:
            return EGG::Vector3f::ex * m_roadVel;
        case 3:
            return -EGG::Vector3f::ex * m_roadVel;
        default:
            return EGG::Vector3f::zero;
        }
    }

    /// @addr{0x807FC6C8}
    [[nodiscard]] bool isMoving(u32 variant, const EGG::Vector3f & /*pos*/) const override {
        return variant == 2 || variant == 3;
    }
};

} // namespace Kinoko::Field
