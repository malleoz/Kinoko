#include "ObjectDokan.hh"

#include "game/field/CollisionDirector.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x807787F0}
ObjectDokan::ObjectDokan(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

/// @addr{0x80778FEC}
ObjectDokan::~ObjectDokan() = default;

/// @addr{0x80778830}
void ObjectDokan::init() {
    m_isAirborne = false;
}

/// @addr{0x807788C8}
void ObjectDokan::calc() {
    if (!m_isAirborne) {
        return;
    }

    calcPos();
    calcFloor();
}

/// @addr{0x80778D50}
void ObjectDokan::calcCollisionTransform() {
    if (m_id == ObjectId::DokanSFC) {
        ObjectCollidable::calcCollisionTransform();
    } else {
        // rMR piranhas
        calcTransform();
        EGG::Matrix34f mat = transform();
        mat.setBase(3, mat.translation() + EGG::Vector3f::ey * 300.0f);
        m_collision->transform(mat, scale(), getCollisionTranslation());
    }
}

/// @addr{0x80778C0C}
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

/// @addr{0x80778BA0}
/// @brief If the pipe is not already airborne, induces upwards velocity
void ObjectDokan::tryStartAirborne() {
    constexpr f32 INITIAL_VELOCITY = 100.0f;

    if (!m_isAirborne) {
        m_isAirborne = true;
        m_velocity = INITIAL_VELOCITY * EGG::Vector3f::ey;
    }
}

} // namespace Kinoko::Field
