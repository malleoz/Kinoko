#include "game/field/obj/ObjectBeltCrossing.hh"

namespace Kinoko::Field {

/// @addr{0x807FC764}
/// @copybrief ObjectBelt::ObjectBelt(const System::MapdataGeoObj &)
/// @param params The parameters used to initialize the object
/// @details Initializes road velocity to `28.0f`.
ObjectBeltCrossing::ObjectBeltCrossing(const System::MapdataGeoObj &params) : ObjectBelt(params) {
    m_roadVel = 28.0f;
}

/// @addr{0x807FD8A8}
/// @brief Default virtual destructor
ObjectBeltCrossing::~ObjectBeltCrossing() = default;

/// @addr{0x807FC7D8}
/// @brief Calculates the conveyer belt's velocity at a given position based on its variant
/// @param variant The variant of the conveyer belt
/// @return The velocity of the conveyer belt at the given position and variant
/// @details The magnitude of velocity is @ref m_roadVel. The direction is determined by the
/// variant, with variant 0 moving to the west and variant 1 moving to the east.
EGG::Vector3f ObjectBeltCrossing::calcRoadVelocity(u32 variant, const EGG::Vector3f & /*pos*/,
        u32 /*timeOffset*/) const {
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
/// @copybrief ObjectBelt::isMoving()
/// @param variant The variant of the conveyer belt
/// @return `true` if the variant is 0 or 1, `false` otherwise (does not occur in the base game)
bool ObjectBeltCrossing::isMoving(u32 variant, const EGG::Vector3f & /*pos*/) const {
    return variant == 0 || variant == 1;
}

} // namespace Kinoko::Field
