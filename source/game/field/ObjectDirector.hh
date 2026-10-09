#pragma once

#include "game/field/ObjectDrivableDirector.hh"
#include "game/field/ObjectFlowTable.hh"
#include "game/field/ObjectHitTable.hh"
#include "game/field/obj/ObjectCollidable.hh"

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
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @addr{0x8082A2B4}
    /// @brief Initializes all objects, including those in @ref ObjectDrivableDirector
    void init() {
        for (auto *&obj : m_objects) {
            obj->init();
            obj->calcModel();
        }

        ObjectDrivableDirector::Instance()->init();
    }

    /// @addr{0x8082A8F4}
    /// @brief Runs per-frame calculations for all objects, including those in @ref
    /// ObjectDrivableDirector
    void calc() {
        for (auto *&obj : m_calcObjects) {
            obj->calc();
        }

        for (auto *&obj : m_calcObjects) {
            obj->calcModel();
        }

        ObjectDrivableDirector::Instance()->calc();
    }

    void addObject(ObjectCollidable *obj);

    /// @brief Adds an object to the director without adding it to the list of objects that require
    /// per-frame calculations or collision checks
    /// @param obj The object to add to the director without adding it to @ref calcObjects
    void addObjectNoImpl(ObjectBase *obj) {
        m_objects.push_back(obj);
    }

    /// @addr{0x806C4ED4}
    /// @brief Adds an object to the list of objects that are meant to be accessed directly by other
    /// @param obj The @ref ObjectCollidable to add to the list of managed objects
    void addManagedObject(ObjectCollidable *obj) {
        m_managedObjects.push_back(obj);
    }

    size_t checkKartObjectCollision(Kart::KartObject *kartObj,
            ObjectCollisionConvexHull *convexHull);

    /// @beginGetters

    /// @brief Gets the @ref ObjectFlowTable for the objects
    /// @return Const ref to the @ref ObjectFlowTable for the objects, @ref m_flowTable
    [[nodiscard]] const ObjectFlowTable &flowTable() const {
        return m_flowTable;
    }

    /// @brief Gets the @ref ObjectHitTable for the kart objects
    /// @return Const ref to the @ref ObjectHitTable for the kart objects, @ref m_hitTableKart
    [[nodiscard]] const ObjectHitTable &hitTableKart() const {
        return m_hitTableKart;
    }

    /// @brief Gets the object at the specified collision index
    /// @param idx The index of the collision to retrieve the object for
    /// @return Const pointer to the @ref ObjectCollidable at the specified collision index in @ref
    /// m_collidingObjects
    /// @pre Assumes that @ref checkKartObjectCollision() has been called and returned a number of
    /// collisions greater than the specified index
    [[nodiscard]] const ObjectCollidable *collidingObject(size_t idx) const {
        ASSERT(idx < m_collidingObjects.size());
        return m_collidingObjects[idx];
    }

    /// @brief Gets the reaction at the specified collision index
    /// @param idx The index of the collision to retrieve the reaction for
    /// @return The @ref Kart::Reaction at the specified collision index in @ref m_reactions
    /// @pre Assumes that @ref checkKartObjectCollision() has been called and returned a number of
    /// collisions greater than the specified index
    [[nodiscard]] Kart::Reaction reaction(size_t idx) const {
        ASSERT(idx < m_reactions.size());
        return m_reactions[idx];
    }

    /// @brief Gets the hit depth at the specified collision index
    /// @param idx The index of the collision to retrieve the hit depth for
    /// @return Const reference to the hit depth vector at the specified collision index in @ref
    /// m_hitDepths
    /// @pre Assumes that @ref checkKartObjectCollision() has been called and returned a number of
    /// collisions greater than the specified index
    [[nodiscard]] const EGG::Vector3f &hitDepth(size_t idx) const {
        ASSERT(idx < m_hitDepths.size());
        return m_hitDepths[idx];
    }

    /// @brief Exposes a span of the managed objects' pointers
    /// @return A span of the managed objects' pointers from @ref m_managedObjects
    [[nodiscard]] std::span<ObjectCollidable *const> managedObjects() {
        return std::span<ObjectCollidable *>(m_managedObjects.begin(), m_managedObjects.size());
    }

    /// @brief Exposes the rising water object, so that @ref Kart::KartMove::calcRisingWater can
    /// know if it should run or not
    /// @return A pointer to the rising water object, @ref m_psea
    [[nodiscard]] ObjectPsea *psea() const {
        return m_psea;
    }

    /// @endGetters

    /// @beginSetters

    /// @brief Sets the @ref ObjectPsea instance to expose in the director
    /// @param psea A pointer to the @ref ObjectPsea instance to set
    void setPsea(ObjectPsea *psea) {
        m_psea = psea;
    }

    /// @endSetters

    [[nodiscard]] f32 distAboveRisingWater(f32 offset) const;
    [[nodiscard]] f32 risingWaterKillPlaneHeight() const;

    ///< @addr{0x808C70E8}
    /// @brief Gets the maximum pitch angle of @ref ObjectWanwan based on the current stage
    /// @return The maximum pitch angle of @ref ObjectWanwan
    /// @see s_wanwanMaxPitch
    [[nodiscard]] static f32 WanwanMaxPitch() {
        return s_wanwanMaxPitch;
    }

    /// @addr{0x8082A784}
    /// @brief Creates the singleton instances of @ref ObjectDirector and @ref
    /// ObjectDrivableDirector. Also creates all objects in the course.
    /// @return A pointer to the newly created singleton instance of @ref ObjectDirector
    static ObjectDirector *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<ObjectDirector>();
        ObjectDrivableDirector::CreateInstance();
        s_instance->createObjects();
        return s_instance;
    }

    /// @addr{0x8082A824}
    /// @brief Destroys the singleton instances of @ref ObjectDirector and @ref
    /// ObjectDrivableDirector
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
        ObjectDrivableDirector::DestroyInstance();
    }

    /// @brief Returns the singleton instance of the @ref ObjectDirector
    /// @return A pointer to the singleton instance of the @ref ObjectDirector
    [[nodiscard]] static ObjectDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    ObjectDirector();

    /// @addr{0x8082A694}
    /// @brief Private virtual destructor that deletes all objects in the director
    ~ObjectDirector() override {
        if (s_instance) {
            s_instance = nullptr;
            WARN("ObjectDirector instance not explicitly handled!");
        }

        for (auto *&obj : m_objects) {
            EGG::egg_delete(obj);
        }
    }

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
