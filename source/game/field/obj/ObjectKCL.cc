#include "ObjectKCL.hh"

#include "game/system/RaceManager.hh"
#include "game/system/ResourceManager.hh"

#include <egg/math/Math.hh>

namespace Kinoko::Field {

/// @addr{0x8081A980}
ObjectKCL::ObjectKCL(const System::MapdataGeoObj &params)
    : ObjectDrivable(params), m_lastMtxUpdateFrame(-2000), m_lastScaleUpdateFrame(-2000) {}

/// @addr{0x8067EAFC}
ObjectKCL::~ObjectKCL() {
    EGG::egg_delete(m_objColMgr);
}

/// @brief Loads the KCL file for the object and creates an @ref ObjColMgr to interface with it
/// @addr{0x8081AA58}
void ObjectKCL::createCollision() {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s.kcl", getKclName());

    auto *resMgr = System::ResourceManager::Instance();
    m_objColMgr =
            EGG::egg_new<ObjColMgr>(resMgr->getFile(filepath, nullptr, System::ArchiveId::Course));
}

/// @addr{0x80681490}
void ObjectKCL::calcCollisionTransform() {
    update(0);
}

/// @addr{0x8081AB4C}
/// @details Saves the initial transformation matrix to the @ref ObjColMgr and computes the KCL's
/// bounding box midpoint and half-width.
void ObjectKCL::initCollision() {
    const EGG::Matrix34f &mat = getUpdatedMatrix(0);
    EGG::Matrix34f matInv;
    bool inverted = mat.ps_inverse(matInv);
    ASSERT(inverted);

    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);
    m_objColMgr->setScale(getScaleY(0));

    EGG::Vector3f high = m_objColMgr->kclHighWorld();
    EGG::Vector3f low = m_objColMgr->kclLowWorld();

    m_kclMidpoint = (high + low).multInv(2.0f);

    EGG::Vector3f highLowDiffAbs = (high - low).abs();
    f32 maxDiff = std::max(highLowDiffAbs.x, highLowDiffAbs.z);
    m_bboxHalfSideLength = maxDiff * 0.5f;
}

/// @addr{0x806810F8}
bool ObjectKCL::checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointPartial(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x806811B0}
bool ObjectKCL::checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointPartialPush(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80681268}
bool ObjectKCL::checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointFull(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80681320}
bool ObjectKCL::checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointFullPush(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680DF4}
bool ObjectKCL::checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    calcScale(timeOffset);
    update(timeOffset);

    return m_objColMgr->checkSpherePartial(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680EF0}
bool ObjectKCL::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    calcScale(timeOffset);
    update(timeOffset);

    return m_objColMgr->checkSpherePartialPush(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680FEC}
bool ObjectKCL::checkSphereFull(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut, u32 timeOffset) {
    calcScale(timeOffset);
    update(timeOffset);

    return m_objColMgr->checkSphereFull(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x806810E8}
bool ObjectKCL::checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkCollision(radius, pos, prevPos, mask, info, maskOut, timeOffset);
}

/// @addr{0x806807E8}
void ObjectKCL::narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
        u32 /*timeOffset*/) {
    m_objColMgr->narrScLocal(radius, pos, mask);
}

/// @addr{0x80680B14}
bool ObjectKCL::checkPointCachedPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointCachedPartial(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680BCC}
bool ObjectKCL::checkPointCachedPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointCachedPartialPush(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680C84}
bool ObjectKCL::checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointCachedFull(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680D3C}
bool ObjectKCL::checkPointCachedFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return m_objColMgr->checkPointCachedFullPush(pos, prevPos, mask, info, maskOut);
}

/// @addr{0x806807F0}
bool ObjectKCL::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedPartial(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x806808FC}
bool ObjectKCL::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedPartialPush(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680A08}
bool ObjectKCL::checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedFull(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80680B04}
bool ObjectKCL::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkCollisionCached(radius, pos, prevPos, mask, info, maskOut, timeOffset);
}

/// @brief Advances the collision manager's transform to reflect the current frame
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
        mat = getUpdatedMatrix(timeOffset);
    }

    EGG::Matrix34f matInv;
    mat.ps_inverse(matInv);
    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);

    m_lastMtxUpdateFrame = time;
}

/// @brief Updates the collision manager's scale to reflect the current frame
/// @addr{0x8081AF28}
void ObjectKCL::calcScale(u32 timeOffset) {
    u32 time = System::RaceManager::Instance()->timer() - timeOffset;
    if (m_lastScaleUpdateFrame == static_cast<s32>(time)) {
        return;
    }

    if (time == 0) {
        m_objColMgr->setScale(scale().y);
    } else {
        m_objColMgr->setScale(getScaleY(timeOffset));
    }

    m_lastScaleUpdateFrame = time;
}

/// @addr{0x8081AFB4}
/// @brief Checks collision between a sphere and the object, writing out full collision info
/// @desync This function can result in physics desynchronizations when racing a ghost.
/// Since this function is only called when the player/ghost's hitbox is close enough to the object
/// (as per GJK collision checks), it is possible that the first player hitbox is just barely too
/// far away from the object while the second player hitbox is just close enough to pass the GJK
/// check and thus call this function and update the collision manager's transform. Subsequently,
/// if the ghost's first hitbox normally would be too far away from the object to pass the GJK
/// check, the transform update from the player's second hitbox check means that the ghost's first
/// hitbox is now being checked against a different transform than the player, which can result in
/// the GJK check passing when it originally would fail. This is the reason that the DS Delfino
/// Square bridge can sometimes desync.
bool ObjectKCL::checkCollision(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut, u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8081B16C}
/// @brief Checks collision between a sphere and the object, using only cached KCL prisms, writing
/// out full collision info
/// @desync This function can result in physics desynchronizations when racing a ghost.
/// Since this function is only called when the player/ghost's hitbox is close enough to the object
/// (as per GJK collision checks), it is possible that the first player hitbox is just barely too
/// far away from the object while the second player hitbox is just close enough to pass the GJK
/// check and thus call this function and update the collision manager's transform. Subsequently,
/// if the ghost's first hitbox normally would be too far away from the object to pass the GJK
/// check, the transform update from the player's second hitbox check means that the ghost's first
/// hitbox is now being checked against a different transform than the player, which can result in
/// the GJK check passing when it originally would fail. This is the reason that the DS Delfino
/// Square bridge can sometimes desync.
bool ObjectKCL::checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut);
}

} // namespace Kinoko::Field
