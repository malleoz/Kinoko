#include "ObjectCollidable.hh"

#include "game/field/ObjectCollisionBox.hh"
#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/ObjectCollisionSphere.hh"
#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8081EFEC}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &params)
    : ObjectBase(params), m_collision(nullptr) {}

/// @addr{0x8081F064}
/// @brief Constructor
/// @param name The name of the object
/// @param pos The initial position of the object
/// @param rot The initial rotation of the object
/// @param scale The initial scale of the object

ObjectCollidable::ObjectCollidable(const char *name, const EGG::Vector3f &pos,
        const EGG::Vector3f &rot, const EGG::Vector3f &scale)
    : ObjectBase(name, pos, rot, scale), m_collision(nullptr) {}

/// @addr{0x8067E384}
/// @brief Default virtual destructor that destroys the associated collision object
ObjectCollidable::~ObjectCollidable() {
    EGG::egg_delete(m_collision);
}

/// @addr{0x8081F0A0}
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
/// @brief Finds the radius that fits fully in a BoxColUnit.
/// @details We refer to the collision parameters as a box due to its use of axes.
/// This does not imply that all collidable objects are boxes!
f32 ObjectCollidable::getCollisionRadius() const {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto *collisionSet = flowTable.set(flowTable.slot(id()));

    f32 zRadius = scale().z * static_cast<f32>(parse<s16>(collisionSet->params.box.z));
    f32 xRadius = scale().x * static_cast<f32>(parse<s16>(collisionSet->params.box.x));

    return std::max(xRadius, zRadius);
}

/// @brief Runs on collision to conditionally modify the hit reaction applied on the player
/// @addr{0x8081F66C}
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
