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
    ObjectCollidable(const System::MapdataGeoObj &params);
    ObjectCollidable(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    ~ObjectCollidable() override;

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
    virtual void loadAABB(f32 maxSpeed) {
        loadAABB(getCollisionRadius(), maxSpeed);
    }

    /// @addr{0x8081F180}
    /// @brief Created a BoxColUnit based off the provided radius and maxSpeed
    virtual void loadAABB(f32 radius, f32 maxSpeed) {
        auto *boxColMgr = BoxColManager::Instance();
        const EGG::Vector3f &pos = getPosition();
        bool alwaysRecalc = loadFlags() & 0x5;
        m_boxColUnit = boxColMgr->insertObject(radius, maxSpeed, &pos, alwaysRecalc, this);
    }

    virtual void processKartReactions(Kart::KartObject *kartObj, Kart::Reaction &reactionOnKart,
            Kart::Reaction &reactionOnObj);

    /// @addr{0x8068179C}
    /// @brief Called when a collision occurs between a kart and this object
    /// @return The reaction that should be applied to the kart
    virtual Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/,
            Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
            EGG::Vector3f & /*hitDepth*/) {
        return reactionOnKart;
    }

    /// @brief Called when a wall-like collision occurs
    virtual void onWallCollision(Kart::KartObject *, const EGG::Vector3f &) {}

    /// @brief Called when a non-wall collision occurs
    virtual void onObjectCollision(Kart::KartObject *) {}

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
    [[nodiscard]] virtual const EGG::Vector3f &getCollisionTranslation() const {
        return EGG::Vector3f::zero;
    }

    /// @addr{0x80573518}
    [[nodiscard]] virtual ObjectCollisionBase *collision() const {
        return m_collision;
    }

protected:
    void createCollision() override;

    /// @addr{0x806816B8}
    /// @brief Defines a local offset for the GJK collision object
    [[nodiscard]] virtual const EGG::Vector3f &collisionCenter() const {
        return EGG::Vector3f::zero;
    }

    void registerManagedObject();

    ObjectCollisionBase *m_collision; ///< GJK collision object
};

} // namespace Field

} // namespace Kinoko
