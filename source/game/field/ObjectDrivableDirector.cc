#include "ObjectDrivableDirector.hh"

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

ObjectDrivableDirector *ObjectDrivableDirector::s_instance = nullptr;

// Explicit instantiation, since callers of checkSphereImpl()/checkSphereCachedImpl() live in the
// header and would otherwise be unable to see these definitions.
template bool ObjectDrivableDirector::checkSphereImpl<CollisionInfoPartial>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset,
        DrivableCheckFunc<CollisionInfoPartial> checkFunc);
template bool ObjectDrivableDirector::checkSphereImpl<CollisionInfo>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut, u32 timeOffset,
        DrivableCheckFunc<CollisionInfo> checkFunc);
template bool ObjectDrivableDirector::checkSphereCachedImpl<CollisionInfoPartial>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset,
        DrivableCheckFunc<CollisionInfoPartial> cachedCheckFunc,
        DrivableCheckFunc<CollisionInfoPartial> checkFunc);
template bool ObjectDrivableDirector::checkSphereCachedImpl<CollisionInfo>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut, u32 timeOffset,
        DrivableCheckFunc<CollisionInfo> cachedCheckFunc,
        DrivableCheckFunc<CollisionInfo> checkFunc);

} // namespace Kinoko::Field
