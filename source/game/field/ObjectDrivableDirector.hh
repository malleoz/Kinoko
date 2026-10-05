#pragma once

#include "game/field/obj/ObjectObakeManager.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

/// @brief Singleton that manages all @ref ObjectDrivable instances and their collision interactions
/// @details Distinguishes between objects that require per-frame calculations and those that do
/// not. Exposes multiple interfaces to perform collision queries similar to that of @ref
/// CourseColMgr, which call into each object's respective virtual collision check method.
/// Separately owns the @ref ObjectObakeManager, which handles SNES Ghost Valley 2 block
/// management and spatial indexing for drivable objects.
class ObjectDrivableDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @addr{0x8081B500}
    /// @brief Initializes all objects and updates their transforms
    void init() {
        for (auto *&obj : m_objects) {
            obj->init();
            obj->calcModel();
        }
    }

    /// @addr{0x8081B618}
    /// @brief Runs per-frame calculations for all objects that require it, and updates their
    /// transforms
    void calc() {
        for (auto *&obj : m_calcObjects) {
            obj->calc();
        }

        for (auto *&obj : m_calcObjects) {
            obj->calcModel();
        }
    }

    /// @addr{0x8081B6C8}
    /// @brief Registers a new @ref ObjectDrivable with the director, and adds it to the list of
    /// objects that require per-frame calculations if applicable
    /// @param obj The @ref ObjectDrivable instance to add
    /// @details Adds the object to @ref m_objects. If the object has the @ref
    /// ObjectBase::eLoadFlags::Calc flag set, also adds it to @ref m_calcObjects.
    void addObject(ObjectDrivable *obj) {
        if (obj->loadFlags().onBit(ObjectBase::eLoadFlags::Calc)) {
            m_calcObjects.push_back(obj);
        }

        m_objects.push_back(obj);
    }

    /// @brief Creates the rGV2 block manager. Also implicitly adds the block represented by params.
    /// @param params The parameters used to initialize the object
    /// @pre Assumes that no block manager has been created yet.
    void createObakeManager(const System::MapdataGeoObj &params) {
        ASSERT(!m_obakeManager);
        m_obakeManager = EGG::egg_new<ObjectObakeManager>(params);
        m_obakeManager->load();
    }

    /// @addr{0x8081BC98}
    /// @brief Checks collision between a sphere and objects, writing partial collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset The time offset to use for the collision check
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info, maskOut,
                timeOffset, &ObjectDrivable::checkSpherePartial);
    }

    /// @addr{0x8081BD70}
    /// @brief Checks collision between a sphere and objects, writing partial collision info.
    /// Additionally pushes the collision entry into the @ref CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset The time offset to use for the collision check
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info, maskOut,
                timeOffset, &ObjectDrivable::checkSpherePartialPush);
    }

    /// @addr{0x8081BE48}
    /// @brief Checks collision between a sphere and objects, writing full collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset The time offset to use for the collision check
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfo>(radius, pos, prevPos, mask, info, maskOut, timeOffset,
                &ObjectDrivable::checkSphereFull);
    }

    /// @addr{0x8081BFA0}
    /// @brief Checks collision between a sphere and objects, writing full collision info.
    /// Additionally pushes the collision entry into the @ref CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset The time offset to use for the collision check
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfo>(radius, pos, prevPos, mask, info, maskOut, timeOffset,
                &ObjectDrivable::checkSphereFullPush);
    }

    /// @addr{0x8081C5A0}
    /// @brief Checks collision between a sphere and objects by using the @ref BoxColManager's
    /// local spatial cache, writing only partial collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset The time offset to use for the collision check
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereCachedImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info,
                maskOut, timeOffset, &ObjectDrivable::checkSphereCachedPartial,
                &ObjectDrivable::checkSpherePartial);
    }

    /// @addr{0x8081C6B4}
    /// @brief Checks collision between a sphere and objects by using the @ref BoxColManager's
    /// local spatial cache, writing only partial collision info. Additionally pushes the collision
    /// entry into the @ref CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset The time offset to use for the collision check
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereCachedImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info,
                maskOut, timeOffset, &ObjectDrivable::checkSphereCachedPartialPush,
                &ObjectDrivable::checkSpherePartialPush);
    }

    /// @addr{0x8081C958}
    /// @brief Checks collision between a sphere and objects by using the @ref BoxColManager's
    /// local spatial cache, writing full collision info. Additionally pushes the collision
    /// entry into the @ref CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset The time offset to use for the collision check
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereCachedImpl<CollisionInfo>(radius, pos, prevPos, mask, info, maskOut,
                timeOffset, &ObjectDrivable::checkSphereCachedFullPush,
                &ObjectDrivable::checkSphereFullPush);
    }

    void colNarScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask, u32 timeOffset);

    /// @beginGetters

    /// @brief Gets the SNES Ghost Valley 2 block manager object
    /// @return A pointer to the @ref ObjectObakeManager instance
    [[nodiscard]] ObjectObakeManager *obakeManager() const {
        return m_obakeManager;
    }

    /// @endGetters

    /// @addr{0x8081B428}
    /// @brief Creates the singleton instance of @ref ObjectDrivableDirector
    /// @return A pointer to the newly created singleton instance of @ref ObjectDrivableDirector
    /// @pre Assumes that no singleton instance of @ref ObjectDrivableDirector has been created yet.
    static ObjectDrivableDirector *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<ObjectDrivableDirector>();
        return s_instance;
    }

    /// @addr{0x8081B4B0}
    /// @brief Destroys the singleton instance of the @ref ObjectDrivableDirector
    /// @pre Assumes that a singleton instance of @ref ObjectDrivableDirector has already been
    /// created.
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref ObjectDrivableDirector
    /// @return A pointer to the singleton instance of the @ref ObjectDrivableDirector
    [[nodiscard]] static ObjectDrivableDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    /// @addr{0x8081B324}
    /// @brief Private constructor
    /// @details Initializes @ref m_object and @ref m_calcObjects to have a maximum size of @ref
    /// MAX_OBJECTS and sets @ref m_obakeManager to `nullptr`.
    ObjectDrivableDirector()
        : m_objects(MAX_OBJECTS),
          m_calcObjects(MAX_OBJECTS),
          m_obakeManager(nullptr) {}

    /// @addr{0x8081B380}
    /// @brief Private virtual destructor that deletes all objects in the director
    ~ObjectDrivableDirector() override {
        if (s_instance) {
            s_instance = nullptr;
            WARN("ObjectDrivableDirector instance not explicitly handled!");
        }

        for (auto *&obj : m_objects) {
            EGG::egg_delete(obj);
        }
    }

    /// @brief Function pointer type for an @ref ObjectDrivable sphere collision check
    /// @tparam T The collision info object type, either @ref CollisionInfo or @ref
    /// CollisionInfoPartial
    template <typename T>
    using DrivableCheckFunc = bool (ObjectDrivable::*)(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset, DrivableCheckFunc<T> checkFunc);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereCachedImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset, DrivableCheckFunc<T> cachedCheckFunc, DrivableCheckFunc<T> checkFunc);

    fixed_vector<ObjectDrivable *> m_objects;     ///< All drivable objects live here
    fixed_vector<ObjectDrivable *> m_calcObjects; ///< Objects needing calc() live here too
    ObjectObakeManager *m_obakeManager;           ///< Manages rGV2 blocks and spatial indexing

    static constexpr size_t MAX_OBJECTS = 400; ///< Maximum number of objects in the vectors

    /// @addr{0x809C4310}
    /// @brief The pointer to the singleton instance of the class
    static ObjectDrivableDirector *s_instance;
};

} // namespace Field

} // namespace Kinoko
