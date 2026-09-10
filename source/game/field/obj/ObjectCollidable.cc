#include "ObjectCollidable.hh"

#include "game/field/ObjectCollisionBox.hh"
#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/ObjectCollisionSphere.hh"
#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8081F0A0}
/// @copybrief ObjectBase::load()
/// @details This function loads the graphical assets, animations, collision object(s), and rail for
/// the collidable object. It also registers the object with the @refObjectDirector.
void ObjectCollidable::load() {
    loadGraphics();
    loadAnims();
    createCollision();

    if (m_collision) {
        loadAABB(0.0f);
    }

    loadRail();

    ObjectDirector::Instance()->addObject(this);
}

/// @addr{0x806815A0}
/// @copybrief ObjectBase::getCollisionRadius()
/// @return The collision radius of the object, calculated based on its bounding box.
/// @details We refer to the collision parameters as a box due to its use of axes.
/// This does not imply that all collidable objects are boxes!
f32 ObjectCollidable::getCollisionRadius() const {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto *collisionSet = flowTable.set(flowTable.slot(id()));

    f32 zRadius = scale().z * static_cast<f32>(parse<s16>(collisionSet->params.box.z));
    f32 xRadius = scale().x * static_cast<f32>(parse<s16>(collisionSet->params.box.x));

    return std::max(xRadius, zRadius);
}

/// @addr{0x8081F66C}
/// @brief Runs on collision to conditionally modify the hit reaction applied on the player
/// @param kartObj The kart object involved in the collision
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @param reactionOnObj The reaction that should be applied to the object upon collision
/// @details If the kart is driving under 50% of its base speed, then it changes @ref
/// Kart::Reaction::SpinTwice to @ref Kart::Reaction::Wall and @ref Kart::Reaction::SpinHitSomeSpeed
/// to @ref Kart::Reaction::None. Otherwise, if the kart is driving at 50% or more of its base
/// speed, it changes @ref Kart::Reaction::SpinHitSomeSpeed to @ref Kart::Reaction::SpinTwice.
void ObjectCollidable::processKartReactions(Kart::KartObject *kartObj,
        Kart::Reaction &reactionOnKart, Kart::Reaction &reactionOnObj) {
    // Process the reaction on kart
    if (kartObj->speedRatioCapped() < 0.5f) {
        if (reactionOnKart == Kart::Reaction::SpinTwice) {
            reactionOnKart = Kart::Reaction::Wall;
        } else if (reactionOnKart == Kart::Reaction::SpinHitSomeSpeed) {
            reactionOnKart = Kart::Reaction::None;
        }
    } else {
        if (reactionOnKart == Kart::Reaction::SpinHitSomeSpeed) {
            reactionOnKart = Kart::Reaction::SpinTwice;
        }
    }

    // Process the reaction on object
    if (reactionOnObj == Kart::Reaction::UNK_3 || reactionOnObj == Kart::Reaction::UNK_4) {
        reactionOnObj = Kart::Reaction::None;
    }
}

/// @addr{0x8081F224}
/// @copybrief ObjectBase::createCollision()
/// @details References the object's @ref SObjectCollisionSet::mode to determine what type of object
/// to create. It is either a sphere, cylinder, or box collision object. In the base game, this
/// function is a no-op for objects having collision mode 0. To better catch instances where an
/// object may have overridden this function, we instead #PANIC.
void ObjectCollidable::createCollision() {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto *collisionSet = flowTable.set(flowTable.slot(id()));

    if (!collisionSet) {
        PANIC("Invalid object ID when creating primitive collision! ID: %zu",
                static_cast<size_t>(id()));
    }

    switch (static_cast<CollisionMode>(parse<s16>(collisionSet->mode))) {
    case CollisionMode::Sphere:
        m_collision = EGG::egg_new<ObjectCollisionSphere>(
                parse<s16>(collisionSet->params.sphere.radius), collisionCenter());
        break;
    case CollisionMode::Cylinder:
        m_collision = EGG::egg_new<ObjectCollisionCylinder>(
                parse<s16>(collisionSet->params.cylinder.radius),
                parse<s16>(collisionSet->params.cylinder.height), collisionCenter());
        break;
    case CollisionMode::Box:
        m_collision = EGG::egg_new<ObjectCollisionBox>(parse<s16>(collisionSet->params.box.x),
                parse<s16>(collisionSet->params.box.y), parse<s16>(collisionSet->params.box.z),
                collisionCenter());
        break;
    default:
        PANIC("Invalid collision mode when creating primitive collision! ID: %zu; Mode: %d",
                static_cast<size_t>(id()), parse<s16>(collisionSet->mode));
        break;
    }
}

/// @addr{0x8081F170}
/// @brief Registers this object to the ObjectDirector's vector of managed objects
void ObjectCollidable::registerManagedObject() {
    ObjectDirector::Instance()->addManagedObject(this);
}

} // namespace Kinoko::Field
