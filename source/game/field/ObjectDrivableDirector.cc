#include "ObjectDrivableDirector.hh"

#include "game/field/BoxColManager.hh"
#include "game/field/obj/ObjectObakeManager.hh"

namespace Kinoko::Field {

/// @brief Helper function which contains frequently re-used code shared by the non-cached
/// `checkSphere*` family of collision check functions
/// @tparam T The collision info object type, either @ref CollisionInfo or @ref
/// CollisionInfoPartial
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset The time offset to use for the collision check
/// @param checkFunc The @ref ObjectDrivable member function used for the collision check
/// @return Whether a collision was detected
/// @details Searches for all colliding @ref BoxColUnit instances having the @ref
/// eBoxColFlag::Drivable flag set that lie within the specified `radius` around provided a position
/// `pos`. For each @ref ObjectDrivable instance found, calls `checkFunc` to determine if a
/// collision occurs.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectDrivableDirector::checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        u32 timeOffset, DrivableCheckFunc<T> checkFunc) {
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, pos, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        hasCollision |= (obj->*checkFunc)(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    return hasCollision;
}

/// @brief Helper function which contains frequently re-used code shared by the cached
/// `checkSphereCached*` family of collision check functions
/// @tparam T The collision info object type, either @ref CollisionInfo or @ref
/// CollisionInfoPartial
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset The time offset to use for the collision check
/// @param cachedCheckFunc The @ref ObjectDrivable member function used for the cached collision
/// check
/// @param checkFunc The @ref ObjectDrivable member function used as a fallback when the sphere is
/// not in the local spatial cache
/// @return Whether a collision was detected
/// @details If the sphere defined by the provided `radius` and `pos` lies within the @ref
/// BoxColManager local spatial cache, then resets the iterator and calls the cached collision check
/// function `cachedCheckFunc`. Otherwise, calls @ref checkSphereImpl() to perform a non-cached
/// collision check.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectDrivableDirector::checkSphereCachedImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        u32 timeOffset, DrivableCheckFunc<T> cachedCheckFunc, DrivableCheckFunc<T> checkFunc) {
    if (m_objects.empty()) {
        return false;
    }

    auto *boxColMgr = BoxColManager::Instance();

    if (boxColMgr->isSphereInSpatialCache(radius, pos, eBoxColFlag::Drivable)) {
        boxColMgr->resetIterators();

        bool hasCollision = false;
        while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
            hasCollision |=
                    (obj->*cachedCheckFunc)(radius, pos, prevPos, mask, info, maskOut, timeOffset);
        }

        return hasCollision;
    }

    return checkSphereImpl<T>(radius, pos, prevPos, mask, info, maskOut, timeOffset, checkFunc);
}

/// @addr{0x8081B500}
/// @brief Initializes all objects and updates their transforms
void ObjectDrivableDirector::init() {
    for (auto *&obj : m_objects) {
        obj->init();
        obj->calcModel();
    }
}

/// @addr{0x8081B618}
/// @brief Runs per-frame calculations for all objects that require it, and updates their
/// transforms
void ObjectDrivableDirector::calc() {
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
void ObjectDrivableDirector::addObject(ObjectDrivable *obj) {
    if (obj->loadFlags().onBit(ObjectBase::eLoadFlags::Calc)) {
        m_calcObjects.push_back(obj);
    }

    m_objects.push_back(obj);
}

/// @brief Creates the rGV2 block manager. Also implicitly adds the block represented by params.
/// @param params The parameters used to initialize the object
/// @pre Assumes that no block manager has been created yet.
void ObjectDrivableDirector::createObakeManager(const System::MapdataGeoObj &params) {
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
bool ObjectDrivableDirector::checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
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
bool ObjectDrivableDirector::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
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
bool ObjectDrivableDirector::checkSphereFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
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
bool ObjectDrivableDirector::checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
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
bool ObjectDrivableDirector::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSphereCachedImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info, maskOut,
            timeOffset, &ObjectDrivable::checkSphereCachedPartial,
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
bool ObjectDrivableDirector::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSphereCachedImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info, maskOut,
            timeOffset, &ObjectDrivable::checkSphereCachedPartialPush,
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
bool ObjectDrivableDirector::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkSphereCachedImpl<CollisionInfo>(radius, pos, prevPos, mask, info, maskOut,
            timeOffset, &ObjectDrivable::checkSphereCachedFullPush,
            &ObjectDrivable::checkSphereFullPush);
}

/// @addr{0x8081B7CC}
/// @brief Narrows the spatial cache in @ref BoxColManager to only include collision defined
/// by the provided mask within a certain radius of the given position.
/// @param radius The radius of the sphere to check within
/// @param pos The point of the sphere to check within
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param timeOffset The time offset to use for the collision check
/// @details For each @ref ObjectDrivable instance found within the spatial cache, calls @ref
/// ObjectDrivable::narrScLocal(). In practice, this only affects @ref ObjectKCL objects, which will
/// dispatch to @ref ObjColMgr::narrScLocal() which will narrow the course's @ref KColData prism
/// cache.
void ObjectDrivableDirector::colNarScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
        u32 timeOffset) {
    if (m_objects.empty()) {
        return;
    }

    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, pos, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        obj->narrScLocal(radius, pos, mask, timeOffset);
    }
}

/// @brief Gets the SNES Ghost Valley 2 block manager object
/// @return A pointer to the @ref ObjectObakeManager instance, @ref m_obakeManager
ObjectObakeManager *ObjectDrivableDirector::obakeManager() const {
    return m_obakeManager;
}

/// @brief Checks whether any drivable objects require per-frame calculations
/// @return `true` if @ref m_calcObjects is non-empty, `false` otherwise
bool ObjectDrivableDirector::hasCalcObjects() const {
    return !m_calcObjects.empty();
}

/// @endGetters

/// @addr{0x8081B428}
/// @brief Creates the singleton instance of @ref ObjectDrivableDirector
/// @return A pointer to the newly created singleton instance of @ref ObjectDrivableDirector
/// @pre Assumes that no singleton instance of @ref ObjectDrivableDirector has been created yet.
ObjectDrivableDirector *ObjectDrivableDirector::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<ObjectDrivableDirector>();
    return s_instance;
}

/// @addr{0x8081B4B0}
/// @brief Destroys the singleton instance of the @ref ObjectDrivableDirector
/// @pre Assumes that a singleton instance of @ref ObjectDrivableDirector has already been
/// created.
void ObjectDrivableDirector::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Returns the singleton instance of the @ref ObjectDrivableDirector
/// @return A pointer to the singleton instance of the @ref ObjectDrivableDirector
ObjectDrivableDirector *ObjectDrivableDirector::Instance() {
    return s_instance;
}

/// @addr{0x8081B324}
/// @brief Private constructor
/// @details Initializes @ref m_object and @ref m_calcObjects to have a maximum size of @ref
/// MAX_OBJECTS and sets @ref m_obakeManager to `nullptr`.
ObjectDrivableDirector::ObjectDrivableDirector()
    : m_objects(MAX_OBJECTS),
      m_calcObjects(MAX_OBJECTS),
      m_obakeManager(nullptr) {}

/// @addr{0x8081B380}
/// @brief Private virtual destructor that deletes all objects in the director
ObjectDrivableDirector::~ObjectDrivableDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ObjectDrivableDirector instance not explicitly handled!");
    }

    for (auto *&obj : m_objects) {
        EGG::egg_delete(obj);
    }
}

ObjectDrivableDirector *ObjectDrivableDirector::s_instance = nullptr;

} // namespace Kinoko::Field
