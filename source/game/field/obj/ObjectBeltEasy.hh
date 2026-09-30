#pragma once

#include "game/field/obj/ObjectBelt.hh"

namespace Kinoko::Field {

/// @brief The conveyers outside of the factory on Toad's Factory.
class ObjectBeltEasy final : public ObjectBelt {
public:
    /// @addr{0x807FC578}
    /// @copybrief ObjectBelt::ObjectBelt(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes the road velocity to `20.0f`.
    ObjectBeltEasy(const System::MapdataGeoObj &params) : ObjectBelt(params) {
        m_roadVel = 20.0f;
    }

    /// @addr{0x807FD8F0}
    /// @brief Default virtual destructor
    ~ObjectBeltEasy() override = default;

private:
    /// @addr{0x807FC62C}
    /// @brief Calculates the conveyer belt's velocity at a given position based on its variant
    /// @param variant The variant of the conveyer belt
    /// @return The velocity of the conveyer belt at the given position and variant
    /// @details The magnitude of velocity is @ref m_roadVel. The direction is determined by the
    /// variant, with variant 2 moving to the east and variant 3 moving to the west.
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
    /// @brief Determines whether or not the conveyer belt is moving based on its variant
    /// @param variant The variant of the conveyer belt
    /// @return `true` if the variant is 2 or 3, `false` otherwise (does not occur in the base game)
    [[nodiscard]] bool isMoving(u32 variant, const EGG::Vector3f & /*pos*/) const override {
        return variant == 2 || variant == 3;
    }
};

} // namespace Kinoko::Field
