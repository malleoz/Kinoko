#include "CollisionDirector.hh"

namespace Kinoko::Field {

/// @brief Helper function which contains frequently re-used code shared by the `checkSphere*`
/// family of collision check functions
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter of type `T` for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @param courseCheckFunc The @ref CourseColMgr member function used for the course KCL check
/// @param drivableCheckFunc The @ref ObjectDrivableDirector member function used for the object
/// check
/// @param cached Whether the check should use the local spatial cache for the course KCL check
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache.
/// @return Whether a collision was detected
/// @details Resets `info` to its default state before performing the collision check. If `maskOut`
/// is provided, then it is reset to `KCL_NONE`; additionally, if `push` was also `true`, the @ref
/// CollisionDirector cache is reset as well. Resets @ref CourseColMgr::m_noBounceWallInfo to its
/// default state if it exists.
///
/// Performs a @ref CourseColMgr collision check if this is a cached query or if this is a
/// non-cached query having `mask` not equal to `KCL_NONE`. Regardless of the return of the course
/// KCL check, always performs a @ref ObjectDrivableDirector collision check. If at least one of the
/// two collision checks returned `true`, then `info` and @ref CourseColMgr::m_noBounceWallInfo are
/// updated appropriately (if they are not `nullptr`). Finally, the @ref
/// CourseColMgr::m_noBounceWallInfo is cleared if present, and the function returns a boolean
/// indicating whether or not a collision has occurred.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool CollisionDirector::checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        u32 timeOffset, CourseColCheckFunc<T> courseCheckFunc,
        DrivableColCheckFunc<T> drivableCheckFunc, bool cached, bool push) {
    if (info) {
        info->reset();
    }

    if (maskOut) {
        if (push) {
            resetCollisionEntries(maskOut);
        } else {
            *maskOut = KCL_NONE;
        }
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    // Cached checks always query, uncached checks skip the query entirely when mask is empty
    bool colliding = (mask || cached) &&
            (courseColMgr->*courseCheckFunc)(1.0f, radius, nullptr, pos, prevPos, mask, info,
                    maskOut);

    colliding |= (ObjectDrivableDirector::Instance()->*drivableCheckFunc)(radius, pos, prevPos,
            mask, info, maskOut, timeOffset);

    if (colliding) {
        if (info) {
            info->tangentOff = info->bbox.min + info->bbox.max;
        }

        if (noBounceInfo) {
            noBounceInfo->tangentOff = noBounceInfo->bbox.min + noBounceInfo->bbox.max;
        }
    }

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @addr{0x807BD96C}
/// @brief Finds the closest KCL triangle out of the list of tris we are colliding with
/// @param type Filters the result for particular KCL types
/// @return Whether there was a collision entry for the provided type
/// @details If an entry was found, then @ref m_closestCollisionEntry will point to the closest
/// collision entry matching the specified type.
bool CollisionDirector::findClosestCollisionEntry(KCLTypeMask * /*typeMask*/, KCLTypeMask type) {
    m_closestCollisionEntry = nullptr;
    f32 minDist = -std::numeric_limits<f32>::min();

    for (size_t i = 0; i < m_collisionEntryCount; ++i) {
        const auto &entry = m_entries[i];
        u32 typeMask = entry.typeMask & type;
        if (typeMask != 0 && entry.dist > minDist) {
            minDist = entry.dist;
            m_closestCollisionEntry = &entry;
        }
    }

    return !!m_closestCollisionEntry;
}

// Explicit instantiation, since callers of checkSphereImpl() live in the header and would
// otherwise be unable to see this definition when the class's vtable is emitted.
template bool CollisionDirector::checkSphereImpl<CollisionInfo>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut, u32 timeOffset,
        CourseColCheckFunc<CollisionInfo> courseCheckFunc,
        DrivableColCheckFunc<CollisionInfo> drivableCheckFunc, bool cached, bool push);
template bool CollisionDirector::checkSphereImpl<CollisionInfoPartial>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset,
        CourseColCheckFunc<CollisionInfoPartial> courseCheckFunc,
        DrivableColCheckFunc<CollisionInfoPartial> drivableCheckFunc, bool cached, bool push);

CollisionDirector *CollisionDirector::s_instance = nullptr;

} // namespace Kinoko::Field
