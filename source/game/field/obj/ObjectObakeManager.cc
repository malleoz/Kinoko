#include "ObjectObakeManager.hh"

namespace Field {

/// @addr{0x8080B0D8}
ObjectObakeManager::ObjectObakeManager(const System::MapdataGeoObj &params)
    : ObjectDrivable(params) {
    ; // TODO
}

/// @addr{0x8080BEA4}
ObjectObakeManager::~ObjectObakeManager() = default;

/// @addr{0x8080BB28}
void ObjectObakeManager::calc() {
    ;
}

void ObjectObakeManager::calcCollisionTransform() {
    ;
}

/// @addr{0x8080BE34}
bool ObjectObakeManager::checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE54}
bool ObjectObakeManager::checkPointPartialPush(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE64}
bool ObjectObakeManager::checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE74}
bool ObjectObakeManager::checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE34}
bool ObjectObakeManager::checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE38}
bool ObjectObakeManager::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE3C}
bool ObjectObakeManager::checkSphereFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE40}
bool ObjectObakeManager::checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDF4}
bool ObjectObakeManager::checkPointCachedPartial(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE04}
bool ObjectObakeManager::checkPointCachedPartialPush(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE14}
bool ObjectObakeManager::checkPointCachedFull(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE24}
bool ObjectObakeManager::checkPointCachedFullPush(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDE4}
bool ObjectObakeManager::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDE8}
bool ObjectObakeManager::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDEC}
bool ObjectObakeManager::checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDF0}
bool ObjectObakeManager::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080B244}
void ObjectObakeManager::addBlock(const System::MapdataGeoObj &params) {
    ; // TODO - MAKE SURE THIS IS CALLED IN OUR FACTORY FUNCTION
}

/// @addr{0x8080BEE4}
bool ObjectObakeManager::checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    ; // TODO
}

/// @addr{0x8080C41C}
bool ObjectObakeManager::checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    ; // TODO
}

/// @addr{0x8080C980}
bool ObjectObakeManager::checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    ; // TODO
}

/// @addr{0x8080D12C}
bool ObjectObakeManager::checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    ; // TODO
}

} // namespace Field
