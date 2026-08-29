#include "ObjectOilSFC.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x806DD934}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectOilSFC::ObjectOilSFC(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

/// @addr{0x806DD998}
/// @brief Default virtual destructor
ObjectOilSFC::~ObjectOilSFC() = default;

/// @addr{0x806DD9D8}
/// @details If the player is on the ground, they will lose traction and spin out.
Kart::Reaction ObjectOilSFC::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) {
    hitDepth = EGG::Vector3f::zero;
    bool touchingGround = kartObj->status().onBit(Kart::eStatus::TouchingGround);
    return touchingGround ? reactionOnKart : Kart::Reaction::None;
}

} // namespace Kinoko::Field
