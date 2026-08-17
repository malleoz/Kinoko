#pragma once

#include "game/field/ObjectCollisionConvexHull.hh"
#include "game/field/ObjectFlowTable.hh"
#include "game/field/ObjectHitTable.hh"
#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectNoImpl.hh"

#include <egg/core/Allocator.hh>

#include <vector>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

class ObjectPsea;

/// @brief Singleton class that manages collidable objects and their interactions
/// @details Distinguishes between objects that require per-frame calculations and those that do
/// not. Also manages collision detection between karts and objects. This class is separate from the
/// @ref ObjectDrivableDirector, which manages objects that can be driven by karts because those
/// objects may implement their own collision logic beyond a simple GJK collision check.
class ObjectDirector : EGG::Disposer {
    friend class Host::Context;

public:
    void init();
    void calc();
    void addObject(ObjectCollidable *obj);

    /// @brief Adds an object to the director without adding it to the list of objects that require
    /// per-frame calculations or collision checks
    void addObjectNoImpl(ObjectBase *obj) {
        m_objects.push_back(obj);
    }

    /// @addr{0x806C4ED4}
    /// @brief Adds an object to the list of objects that are meant to be accessed directly by other
    void addManagedObject(ObjectCollidable *obj) {
        m_managedObjects.push_back(obj);
    }

    size_t checkKartObjectCollision(Kart::KartObject *kartObj,
            ObjectCollisionConvexHull *convexHull);

    /// @beginGetters
    [[nodiscard]] const ObjectFlowTable &flowTable() const {
        return m_flowTable;
    }

    [[nodiscard]] const ObjectHitTable &hitTableKart() const {
        return m_hitTableKart;
    }

    [[nodiscard]] const ObjectCollidable *collidingObject(size_t idx) const {
        ASSERT(idx < m_collidingObjects.size());
        return m_collidingObjects[idx];
    }

    [[nodiscard]] Kart::Reaction reaction(size_t idx) const {
        ASSERT(idx < m_reactions.size());
        return m_reactions[idx];
    }

    [[nodiscard]] const EGG::Vector3f &hitDepth(size_t idx) const {
        ASSERT(idx < m_hitDepths.size());
        return m_hitDepths[idx];
    }

    [[nodiscard]] fixed_vector<ObjectCollidable *> &managedObjects() {
        return m_managedObjects;
    }

    [[nodiscard]] const fixed_vector<ObjectCollidable *> &managedObjects() const {
        return m_managedObjects;
    }

    /// @brief Exposes the rising water object, so that @ref Kart::KartMove::calcRisingWater can
    /// know if it should run or not
    [[nodiscard]] ObjectPsea *psea() const {
        return m_psea;
    }
    /// @endGetters

    /// @beginGetters
    void setPsea(ObjectPsea *psea) {
        m_psea = psea;
    }
    /// @endGetters

    [[nodiscard]] f32 distAboveRisingWater(f32 offset) const;
    [[nodiscard]] f32 risingWaterKillPlaneHeight() const;

    ///< @addr{0x808C70E8}
    [[nodiscard]] static f32 WanwanMaxPitch() {
        return s_wanwanMaxPitch;
    }

    static ObjectDirector *CreateInstance();
    static void DestroyInstance();

    /// @brief Returns the singleton instance of the @ref ObjectDirector
    /// @return A pointer to the singleton instance of the @ref ObjectDirector
    [[nodiscard]] static ObjectDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    ObjectDirector();
    ~ObjectDirector() override;

    void createObjects();
    [[nodiscard]] ObjectBase *createObject(const System::MapdataGeoObj &params);

    ObjectFlowTable m_flowTable;   ///< Maps each @ref ObjectId to its @ref SObjectCollisionSet
    ObjectHitTable m_hitTableKart; ///< Defines player interactions with objects
    ObjectHitTable m_hitTableKartObject; ///< Defines object interactions with players

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
    static f32 s_wanwanMaxPitch;

    static constexpr size_t MAX_MANAGED_OBJECTS = 400; ///< Maximum number of managed objects

    static ObjectDirector *s_instance; ///< @addr{0x809C4330}
};

} // namespace Field

} // namespace Kinoko
