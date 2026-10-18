#pragma once

#include "game/field/ObjectFlowTable.hh"
#include "game/field/ObjectHitTable.hh"

#include <egg/math/Vector.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Kart {

class KartObject;

} // namespace Kart

namespace System {

class MapdataGeoObj;

}

namespace Field {

class ObjectBase;
class ObjectCollidable;
class ObjectCollisionConvexHull;
class ObjectPsea;

/// @brief Singleton class that manages collidable objects and their interactions
/// @details Distinguishes between objects that require per-frame calculations and those that do
/// not. Also manages collision detection between karts and objects. This class is separate from the
/// @ref ObjectDrivableDirector, which manages objects that can be driven by karts because those
/// objects may implement their own collision logic beyond a simple GJK collision check.
class ObjectDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    void init();
    void calc();

    void addObject(ObjectCollidable *obj);
    void addObjectNoImpl(ObjectBase *obj);
    void addManagedObject(ObjectCollidable *obj);

    size_t checkKartObjectCollision(Kart::KartObject *kartObj,
            ObjectCollisionConvexHull *convexHull);

    /// @beginGetters

    [[nodiscard]] const ObjectFlowTable &flowTable() const;
    [[nodiscard]] const ObjectHitTable &hitTableKart() const;
    [[nodiscard]] const ObjectCollidable *collidingObject(size_t idx) const;
    [[nodiscard]] Kart::Reaction reaction(size_t idx) const;
    [[nodiscard]] const EGG::Vector3f &hitDepth(size_t idx) const;
    [[nodiscard]] std::span<ObjectCollidable *const> managedObjects();
    [[nodiscard]] ObjectPsea *psea() const;

    /// @endGetters

    /// @beginSetters

    void setPsea(ObjectPsea *psea);

    /// @endSetters

    [[nodiscard]] f32 distAboveRisingWater(f32 offset) const;
    [[nodiscard]] f32 risingWaterKillPlaneHeight() const;

    [[nodiscard]] static f32 WanwanMaxPitch();

    static ObjectDirector *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static ObjectDirector *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    ObjectDirector();
    ~ObjectDirector() override;

    void createObjects();
    [[nodiscard]] ObjectBase *createObject(const System::MapdataGeoObj &params);

    const ObjectFlowTable m_flowTable;         ///< Maps each object to its @ref SObjectCollisionSet
    const ObjectHitTable m_hitTableKart;       ///< Defines player interactions with objects
    const ObjectHitTable m_hitTableKartObject; ///< Defines object interactions with players

    fixed_vector<ObjectBase *> m_objects;          ///< All objects live here
    fixed_vector<ObjectBase *> m_calcObjects;      ///< Objects needing calc() live here too.
    fixed_vector<ObjectBase *> m_collisionObjects; ///< Objects having collision live here too

    /// @brief The maximum number of objects that can be colliding with a kart at once
    static constexpr size_t MAX_UNIT_COUNT = 200;

    /// @brief Objects we are currently colliding with
    std::array<ObjectCollidable *, MAX_UNIT_COUNT> m_collidingObjects;

    /// @brief For each colliding object, the depth of the collision
    std::array<EGG::Vector3f, MAX_UNIT_COUNT> m_hitDepths;

    /// @brief For each colliding object, the reaction that should be applied to the kart
    std::array<Kart::Reaction, MAX_UNIT_COUNT> m_reactions;

    /// @brief Pointer to the rising water object (like on GCN Peach Beach) if present
    ObjectPsea *m_psea;

    /// @brief Array of objects that are meant to be accessed directly by other objects
    fixed_vector<ObjectCollidable *> m_managedObjects;

    /// @addr{0x808C70E8}
    /// @brief The maximum pitch angle of the Chain Chomp's head when it is moving
    /// @desync @ref ObjectDirector::s_wanwanMaxPitch is used to describe the maximum pitch of the
    /// Chain Chomp. In the base game, this value is statically initialized to -30.0f and is only
    /// overwritten if you load GCN Mario Circuit, at which point it is changed to -20.0f. If you
    /// then load into Mario Circuit Wii, it will continue to be set to the overwritten value of
    /// -20.0f. This means that the Chain Chomp's behavior will desync if GCN Mario Circuit is
    /// loaded before playing Mario Circuit Wii. To compensate for this, we always set the max pitch
    /// in @ref ObjectDirector::createObjects().
    static f32 s_wanwanMaxPitch;

    static constexpr size_t MAX_MANAGED_OBJECTS = 400; ///< Maximum number of managed objects

    /// @addr{0x809C4330}
    /// @brief The pointer to the singleton instance of the class
    static ObjectDirector *s_instance;
};

} // namespace Field

} // namespace Kinoko
