#include "game/field/ObjectDrivableDirector.hh"

namespace Field {

/// @addr{0x8081B500}
void ObjectDrivableDirector::init() {
    for (auto *&obj : m_objects) {
        obj->init();
        obj->calcModel();
    }
}

/// @addr{0x8081B618}
void ObjectDrivableDirector::calc() {
    for (auto *&obj : m_calcObjects) {
        obj->calc();
    }

    for (auto *&obj : m_calcObjects) {
        obj->calcModel();
    }
}

/// @addr{0x8081B6C8}
void ObjectDrivableDirector::addObject(ObjectDrivable *obj) {
    if (obj->loadFlags() & 1) {
        m_calcObjects.push_back(obj);
    }

    m_objects.push_back(obj);
}

/// @addr{0x8081BC98}
bool ObjectDrivableDirector::checkSpherePartial(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask mask, CollisionInfo *colInfo, KCLTypeMask *maskOut,
        u32 start) {
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    BoxColManager::Instance()->search(radius, v0, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = BoxColManager::Instance()->getNextDrivable()) {
        hasCollision |= obj->checkSpherePartial(radius, v0, v1, mask, colInfo, maskOut, start);
    }

    return hasCollision;
}

/// @addr{0x8081BD70}
bool ObjectDrivableDirector::checkSpherePartialPush(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask mask, CollisionInfo *colInfo, KCLTypeMask *maskOut,
        u32 start) {
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    BoxColManager::Instance()->search(radius, v0, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = BoxColManager::Instance()->getNextDrivable()) {
        hasCollision |= obj->checkSpherePartialPush(radius, v0, v1, mask, colInfo, maskOut, start);
    }

    return hasCollision;
}

/// @addr{0x8081BE48}
bool ObjectDrivableDirector::checkSphereFull(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask mask, CollisionInfo *colInfo, KCLTypeMask *maskOut,
        u32 start) {
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    BoxColManager::Instance()->search(radius, v0, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = BoxColManager::Instance()->getNextDrivable()) {
        hasCollision |= obj->checkSphereFull(radius, v0, v1, mask, colInfo, maskOut, start);
    }

    return hasCollision;
}

/// @addr{0x8081BFA0}
bool ObjectDrivableDirector::checkSphereFullPush(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask mask, CollisionInfo *colInfo, KCLTypeMask *maskOut,
        u32 start) {
    if (m_objects.empty()) {
        return false;
    }

    bool hasCollision = false;
    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, v0, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        hasCollision |= obj->checkSphereFullPush(radius, v0, v1, mask, colInfo, maskOut, start);
    }

    return hasCollision;
}

/// @addr{0x8081C5A0}
bool ObjectDrivableDirector::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask mask, CollisionInfo *colInfo, KCLTypeMask *maskOut,
        u32 start) {
    if (m_objects.empty()) {
        return false;
    }

    auto *boxColMgr = BoxColManager::Instance();

    if (boxColMgr->isPosInSpatialCache(radius, v0, eBoxColFlag::Drivable)) {
        BoxColManager::Instance()->resetIterators();

        bool hasCollision = false;
        while (ObjectDrivable *obj = BoxColManager::Instance()->getNextDrivable()) {
            hasCollision |=
                    obj->checkSphereCachedPartial(radius, v0, v1, mask, colInfo, maskOut, start);
        }

        return hasCollision;
    }

    return checkSpherePartial(radius, v0, v1, mask, colInfo, maskOut, start);
}

/// @addr{0x8081C6B4}
bool ObjectDrivableDirector::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask mask, CollisionInfo *colInfo, KCLTypeMask *maskOut,
        u32 start) {
    if (m_objects.empty()) {
        return false;
    }

    auto *boxColMgr = BoxColManager::Instance();

    if (boxColMgr->isPosInSpatialCache(radius, v0, eBoxColFlag::Drivable)) {
        boxColMgr->resetIterators();

        bool hasCollision = false;
        while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
            hasCollision |= obj->checkSphereCachedPartialPush(radius, v0, v1, mask, colInfo,
                    maskOut, start);
        }

        return hasCollision;
    }

    return checkSpherePartialPush(radius, v0, v1, mask, colInfo, maskOut, start);
}

/// @addr{0x8081C958}
bool ObjectDrivableDirector::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask mask, CollisionInfo *colInfo, KCLTypeMask *maskOut,
        u32 start) {
    if (m_objects.empty()) {
        return false;
    }

    auto *boxColMgr = BoxColManager::Instance();

    if (boxColMgr->isPosInSpatialCache(radius, v0, eBoxColFlag::Drivable)) {
        bool hasCollision = false;
        boxColMgr->resetIterators();

        while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
            hasCollision |=
                    obj->checkSphereCachedFullPush(radius, v0, v1, mask, colInfo, maskOut, start);
        }

        return hasCollision;
    }

    return checkSphereFullPush(radius, v0, v1, mask, colInfo, maskOut, start);
}

/// @addr{0x8081B7CC}
void ObjectDrivableDirector::colNarScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
        bool bScaledUp) {
    if (m_objects.empty()) {
        return;
    }

    auto *boxColMgr = BoxColManager::Instance();
    boxColMgr->search(radius, pos, eBoxColFlag::Drivable);

    while (ObjectDrivable *obj = boxColMgr->getNextDrivable()) {
        obj->narrScLocal(radius, pos, mask, bScaledUp);
    }
}

/// @addr{0x8081B428}
ObjectDrivableDirector *ObjectDrivableDirector::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = new ObjectDrivableDirector;
    return s_instance;
}

/// @addr{0x8081B4B0}
void ObjectDrivableDirector::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    delete instance;
}

ObjectDrivableDirector *ObjectDrivableDirector::Instance() {
    return s_instance;
}

/// @addr{0x8082A38C}
ObjectDrivableDirector::ObjectDrivableDirector() {}

/// @addr{0x8082A694}
ObjectDrivableDirector::~ObjectDrivableDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ObjectDrivableDirector instance not explicitly handled!");
    }
}

ObjectDrivableDirector *ObjectDrivableDirector::s_instance = nullptr; ///< @addr{0x809C4310}

} // namespace Field
