#include "ObjectOilSFC.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartObject.hh"
#include "game/kart/KartState.hh"

namespace Kinoko::Field {

/// @addr{0x806DD934}
ObjectOilSFC::ObjectOilSFC(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

/// @addr{0x806DD998}
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
