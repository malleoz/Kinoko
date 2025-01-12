#include "ObjectKCL.hh"

#include "game/system/RaceManager.hh"
#include "game/system/ResourceManager.hh"

#include <egg/math/Math.hh>

namespace Field {

/// @addr{0x8081A980}
ObjectKCL::ObjectKCL(const System::MapdataGeoObj &params)
    : ObjectDrivable(params), m_lastMtxUpdateFrame(-2000), m_lastScaleUpdateFrame(-2000) {}

/// @addr{0x8067EAFC}
ObjectKCL::~ObjectKCL() = default;

/// @addr{0x80681490}
void ObjectKCL::calcCollisionTransform() {
    update(0);
}

/// @addr{0x8081AA58}
void ObjectKCL::createCollision() {
    const char *name = getModelName();

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s.kcl", name);

    auto *resMgr = System::ResourceManager::Instance();
    m_objColMgr = new ObjColMgr(resMgr->getFile(filepath, nullptr, System::ArchiveId::Course));
}

const EGG::Vector3f &ObjectKCL::getPosition() const {
    return m_kclMidpoint;
}

/// @addr{0x80687D70}
f32 ObjectKCL::getCollisionRadius() const {
    return m_bboxHalfSideLength + colRadiusAdditionalLength();
}

/// @addr{0x8081AB4C}
void ObjectKCL::initCollision() {
    const EGG::Matrix34f mat = getUpdatedMatrix();
    EGG::Matrix34f matInv = mat.ps_inverse();
    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);
    m_objColMgr->setScale(getScaleY());

    EGG::Vector3f high = m_objColMgr->kclHighWorld();
    EGG::Vector3f low = m_objColMgr->kclLowWorld();

    m_kclMidpoint = (high + low) * (1.0f / 2.0f);

    EGG::Vector3f highLowDiff = high - low;
    f32 maxDiff = std::max(EGG::Mathf::abs(highLowDiff.x), EGG::Mathf::abs(highLowDiff.z));
    m_bboxHalfSideLength = maxDiff * 0.5f;
}

/// @addr{0x806810F8}
bool ObjectKCL::checkPointPartial(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    return m_objColMgr->checkPointPartial(v0, v1, flags, &pInfo->bbox, pFlagsOut);
}

/// @addr{0x806811B0}
bool ObjectKCL::checkPointPartialPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColPush()) {
        return false;
    }

    return m_objColMgr->checkPointPartialPush(v0, v1, flags, &pInfo->bbox, pFlagsOut);
}

/// @addr{0x80681268}
bool ObjectKCL::checkPointFull(const EGG::Vector3f &v0, const EGG::Vector3f &v1, KCLTypeMask flags,
        CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    return m_objColMgr->checkPointFull(v0, v1, flags, pInfo, pFlagsOut);
}

/// @addr{0x80681320}
bool ObjectKCL::checkPointFullPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColPush()) {
        return false;
    }

    return m_objColMgr->checkPointFullPush(v0, v1, flags, pInfo, pFlagsOut);
}

/// @addr{0x80680DF4}
bool ObjectKCL::checkSpherePartial(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    calcScale(timeOffset);
    update(timeOffset);

    return m_objColMgr->checkSpherePartial(radius, v0, v1, flags, &pInfo->bbox, pFlagsOut);
}

/// @addr{0x80680EF0}
bool ObjectKCL::checkSpherePartialPush(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset) {
    if (!shouldCheckColPush()) {
        return false;
    }

    calcScale(timeOffset);
    update(timeOffset);

    return m_objColMgr->checkSpherePartialPush(radius, v0, v1, flags, &pInfo->bbox, pFlagsOut);
}

/// @addr{0x80680FEC}
bool ObjectKCL::checkSphereFull(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    calcScale(timeOffset);
    update(timeOffset);

    return m_objColMgr->checkSphereFull(radius, v0, v1, flags, pInfo, pFlagsOut);
}

/// @addr{0x806810E8}
bool ObjectKCL::checkSphereFullPush(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset) {
    return checkCollision(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
}

/// @addr{0x806807E8}
void ObjectKCL::narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
        bool /*bScaledUp*/) {
    m_objColMgr->narrScLocal(radius, pos, mask);
}

/// @addr{0x80680B14}
bool ObjectKCL::checkPointCachedPartial(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    return m_objColMgr->checkPointCachedPartial(v0, v1, flags, &pInfo->bbox, pFlagsOut);
}

/// @addr{0x80680BCC}
bool ObjectKCL::checkPointCachedPartialPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColPush()) {
        return false;
    }

    return m_objColMgr->checkPointCachedPartialPush(v0, v1, flags, &pInfo->bbox, pFlagsOut);
}

/// @addr{0x80680C84}
bool ObjectKCL::checkPointCachedFull(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    return m_objColMgr->checkPointCachedFull(v0, v1, flags, pInfo, pFlagsOut);
}

/// @addr{0x80680D3C}
bool ObjectKCL::checkPointCachedFullPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) {
    if (!shouldCheckColPush()) {
        return false;
    }

    return m_objColMgr->checkPointCachedFullPush(v0, v1, flags, pInfo, pFlagsOut);
}

/// @addr{0x806807F0}
bool ObjectKCL::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut,
        u32 timeOffset) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedPartial(radius, v0, v1, flags, &pInfo->bbox, pFlagsOut);
}

/// @addr{0x806808FC}
bool ObjectKCL::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut,
        u32 timeOffset) {
    if (!shouldCheckColPush()) {
        return false;
    }

    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedPartialPush(radius, v0, v1, flags, &pInfo->bbox,
            pFlagsOut);
}

/// @addr{0x80680A08}
bool ObjectKCL::checkSphereCachedFull(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset) {
    if (!shouldCheckColNoPush()) {
        return false;
    }

    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedFull(radius, v0, v1, flags, pInfo, pFlagsOut);
}

/// @addr{0x80680B04}
bool ObjectKCL::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut,
        u32 timeOffset) {
    return checkCollisionCached(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
}

/// @addr{0x8081AD6C}
void ObjectKCL::update(u32 timeOffset) {
    u32 time = System::RaceManager::Instance()->timer() - timeOffset;
    if (m_lastMtxUpdateFrame == static_cast<s32>(time)) {
        return;
    }

    EGG::Matrix34f mat;

    if (timeOffset == 0) {
        calcTransform();
        mat = transform();
    } else {
        mat = getUpdatedMatrix();
    }

    EGG::Matrix34f matInv = mat.ps_inverse();
    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);

    m_lastMtxUpdateFrame = time;
}

/// @addr{0x8081AF28}
void ObjectKCL::calcScale(u32 timeOffset) {
    u32 time = System::RaceManager::Instance()->timer() - timeOffset;
    if (m_lastScaleUpdateFrame == static_cast<s32>(time)) {
        return;
    }

    if (time == 0) {
        m_objColMgr->setScale(m_scale.y);
    } else {
        m_objColMgr->setScale(getScaleY());
    }

    m_lastScaleUpdateFrame = time;
}

/// @addr{0x807FEAC0}
const EGG::Matrix34f &ObjectKCL::getUpdatedMatrix() {
    calcTransform();
    return transform();
}

/// @addr{0x80687DB0}
f32 ObjectKCL::getScaleY() const {
    return m_scale.y;
}

/// @addr{0x8068143C}
f32 ObjectKCL::colRadiusAdditionalLength() const {
    return 0.0f;
}

/// @addr{0x806809F8}
bool ObjectKCL::shouldCheckColNoPush() const {
    return true;
}

/// @addr{0x806809F8}
bool ObjectKCL::shouldCheckColPush() const {
    return true;
}

/// @addr{0x8081AFB4}
bool ObjectKCL::checkCollision(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset) {
    if (!shouldCheckColPush()) {
        return false;
    }

    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereFullPush(radius, v0, v1, flags, pInfo, pFlagsOut);
}

/// @addr{0x8081B16C}
bool ObjectKCL::checkCollisionCached(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
        KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset) {
    if (!shouldCheckColPush()) {
        return false;
    }

    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedFullPush(radius, v0, v1, flags, pInfo, pFlagsOut);
}

} // namespace Field
