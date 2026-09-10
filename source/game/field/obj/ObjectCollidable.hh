#pragma once

#include "game/field/obj/ObjectBase.hh"

#include "game/field/ObjectCollisionBase.hh"

namespace Kinoko {

namespace Kart {

class KartObject;
enum class Reaction;

} // namespace Kart

namespace Field {

/// @brief %Abstract base class for all objects that can collide with karts
/// @details Declares virtual methods for collision checks and collision callbacks.
class ObjectCollidable : public ObjectBase {
public:
    /// @addr{0x8081EFEC}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectCollidable(const System::MapdataGeoObj &params)
        : ObjectBase(params),
          m_collision(nullptr) {}

    /// @addr{0x8081F064}
    /// @brief Constructor
    /// @param name The name of the object
    /// @param pos The initial position of the object
    /// @param rot The initial rotation of the object
    /// @param scale The initial scale of the object
    ObjectCollidable(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale)
        : ObjectBase(name, pos, rot, scale),
          m_collision(nullptr) {}

    /// @addr{0x8067E384}
    /// @brief Default virtual destructor that destroys the associated collision object
    ~ObjectCollidable() override {
        EGG::egg_delete(m_collision);
    }

    void load() override;

    /// @addr{0x8081F7C8}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details Updates the GJK collision transform
    void calcCollisionTransform() override {
        calcTransform();
        m_collision->transform(transform(), scale(), getCollisionTranslation());
    }

    [[nodiscard]] f32 getCollisionRadius() const override;

    /// @addr{0x806816D8}
    /// @brief Creates a BoxColUnit based off the collision radius and the provided maxSpeed
    /// @param maxSpeed The maximum speed of the object
    virtual void loadAABB(f32 maxSpeed) {
        loadAABB(getCollisionRadius(), maxSpeed);
    }

    /// @addr{0x8081F180}
    /// @brief Creates a @ref BoxColUnit based off the provided radius and maxSpeed
    /// @param radius The collision radius of the object
    /// @param maxSpeed The maximum speed of the object
    virtual void loadAABB(f32 radius, f32 maxSpeed) {
        auto *boxColMgr = BoxColManager::Instance();
        const EGG::Vector3f &pos = getPosition();
        bool alwaysRecalc = loadFlags().onAnyBit(eLoadFlags::Calc, eLoadFlags::Unk);
        m_boxColUnit = boxColMgr->insertObject(radius, maxSpeed, &pos, alwaysRecalc, this);
    }

    virtual void processKartReactions(Kart::KartObject *kartObj, Kart::Reaction &reactionOnKart,
            Kart::Reaction &reactionOnObj);

    /// @addr{0x8068179C}
    /// @brief Called when a collision occurs between a kart and this object
    /// @param reactionOnKart The reaction that should be applied to the kart upon collision
    /// @return The reaction that should be applied to the kart
    virtual Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/,
            Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
            EGG::Vector3f & /*hitDepth*/) {
        return reactionOnKart;
    }

    /// @addr{0x80681748}
    /// @brief Performs a collision check between this object and another collision object
    /// @param lhs The object to check collision against (usually the player)
    /// @param dist If a collision occurs, set to the distance between the two objects
    /// @return Whether or not a collision occurred
    virtual bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) {
        return lhs->check(*collision(), dist);
    }

    /// @addr{0x8068173C}
    /// @brief The translation applied on top of the object's world transform for collision checks
    /// @return The translation vector applied on top of the object's world transform for collision
    /// checks
    [[nodiscard]] virtual const EGG::Vector3f &getCollisionTranslation() const {
        return EGG::Vector3f::zero;
    }

    /// @addr{0x80573518}
    /// @brief Gets a pointer to the GJK collision object
    /// @return A pointer to the @ref ObjectCollisionBase GJK collision object
    /// @details In the base game, this is a virtual function. Since no derived class overrides this
    /// function, we can devirtualize for Kinoko.
    [[nodiscard]] ObjectCollisionBase *collision() const {
        return m_collision;
    }

protected:
    void createCollision() override;

    /// @addr{0x806816B8}
    /// @brief Defines a local center for the GJK collision object
    /// @return The local center of the GJK collision object
    [[nodiscard]] virtual const EGG::Vector3f &collisionCenter() const {
        return EGG::Vector3f::zero;
    }

    void registerManagedObject();

    ObjectCollisionBase *m_collision; ///< Pointer to the associated GJK collision object
};

} // namespace Field

} // namespace Kinoko
