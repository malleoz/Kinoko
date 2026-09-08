#include "ObjectDrivableDirector.hh"

namespace Kinoko::Field {

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
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, pos, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        hasCollision |=
                obj->checkSpherePartial(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    return hasCollision;
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
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, pos, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        hasCollision |=
                obj->checkSpherePartialPush(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    return hasCollision;
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
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, pos, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        hasCollision |= obj->checkSphereFull(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    return hasCollision;
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
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, pos, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        hasCollision |=
                obj->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    return hasCollision;
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
    if (m_objects.empty()) {
        return false;
    }

    auto *boxColMgr = BoxColManager::Instance();

    if (boxColMgr->isSphereInSpatialCache(radius, pos, eBoxColFlag::Drivable)) {
        boxColMgr->resetIterators();

        bool hasCollision = false;
        while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
            hasCollision |= obj->checkSphereCachedPartial(radius, pos, prevPos, mask, info, maskOut,
                    timeOffset);
        }

        return hasCollision;
    }

    return checkSpherePartial(radius, pos, prevPos, mask, info, maskOut, timeOffset);
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
    if (m_objects.empty()) {
        return false;
    }

    auto *boxColMgr = BoxColManager::Instance();

    if (boxColMgr->isSphereInSpatialCache(radius, pos, eBoxColFlag::Drivable)) {
        boxColMgr->resetIterators();

        bool hasCollision = false;
        while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
            hasCollision |= obj->checkSphereCachedPartialPush(radius, pos, prevPos, mask, info,
                    maskOut, timeOffset);
        }

        return hasCollision;
    }

    return checkSpherePartialPush(radius, pos, prevPos, mask, info, maskOut, timeOffset);
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
    if (m_objects.empty()) {
        return false;
    }

    auto *boxColMgr = BoxColManager::Instance();

    if (boxColMgr->isSphereInSpatialCache(radius, pos, eBoxColFlag::Drivable)) {
        bool hasCollision = false;
        boxColMgr->resetIterators();

        while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
            hasCollision |= obj->checkSphereCachedFullPush(radius, pos, prevPos, mask, info,
                    maskOut, timeOffset);
        }

        return hasCollision;
    }

    return checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut, timeOffset);
}

/// @addr{0x8081B7CC}
/// @brief Narrows the spatial cache in @ref BoxColManager to only include collision defined
/// by the provided mask within a certain radius of the given position.
/// @param radius The radius of the sphere to check within
/// @param pos The point of the sphere to check within
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param timeOffset The time offset to use for the collision check
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

/// @addr{0x8081B324}
/// @brief Private constructor
ObjectDrivableDirector::ObjectDrivableDirector()
    : m_objects(MAX_OBJECTS),
      m_calcObjects(MAX_OBJECTS),
      m_obakeManager(nullptr) {}

/// @addr{0x8081B380}
/// @brief Private destructor
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
