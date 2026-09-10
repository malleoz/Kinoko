#include "ObjectDokan.hh"

#include "game/field/CollisionDirector.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x80778C0C}
/// @copydoc ObjectCollidable::onCollision()
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @param reactionOnObj The reaction that should be applied to the object upon collision
/// @return For time trials, always returns @ref Kart::Reaction::Wall3.
Kart::Reaction ObjectDokan::onCollision(Kart::KartObject * /*kartObj*/,
        Kart::Reaction reactionOnKart, Kart::Reaction reactionOnObj, EGG::Vector3f & /*hitDepth*/) {
    if (reactionOnObj == Kart::Reaction::UNK_3 || reactionOnObj == Kart::Reaction::UNK_5) {
        tryStartAirborne();
    }

    return reactionOnKart;
}

/// @addr{0x807789BC}
/// @brief Performs a collision check against the floor to stop the pipe if it's falling
void ObjectDokan::calcFloor() {
    constexpr f32 PIPE_RADIUS = 100.0f;
    constexpr f32 PIPE_SQRT_RADIUS = 10.0f;
    constexpr f32 ACCELERATION = 0.2f;

    CollisionInfo colInfo;
    EGG::Vector3f colPos = pos();
    colPos.y += PIPE_RADIUS;
    KCLTypeMask typeMask;

    if (!CollisionDirector::Instance()->checkSphereFull(PIPE_RADIUS, colPos, EGG::Vector3f::inf,
                KCL_TYPE_64EBDFFF, &colInfo, &typeMask, 0)) {
        return;
    }

    addPos(EGG::Vector3f(0.0f, colInfo.tangentOff.y, 0.0f));

    if (typeMask & KCL_TYPE_FLOOR) {
        m_velocity.y *= -ACCELERATION;
        if (m_velocity.length() < ACCELERATION * PIPE_SQRT_RADIUS) {
            m_isAirborne = false;
        }
    }
}

} // namespace Kinoko::Field
