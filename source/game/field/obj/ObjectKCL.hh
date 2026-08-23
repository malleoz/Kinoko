#pragma once

#include "game/field/ObjColMgr.hh"
#include "game/field/obj/ObjectDrivable.hh"

namespace Kinoko::Field {

/// @brief %Abstract class that represents a drivable object with KCL collision data
class ObjectKCL : public ObjectDrivable {
public:
    ObjectKCL(const System::MapdataGeoObj &params);
    ~ObjectKCL() override;

    void createCollision() override;

    /// @addr{0x80681490}
    void calcCollisionTransform() override {
        update(0);
    }

    /// @addr{0x80681448}
    /// @details Computed as the midpoint of the KCL's bounding box
    [[nodiscard]] const EGG::Vector3f &getPosition() const override {
        return m_kclMidpoint;
    }

    /// @addr{0x80687D70}
    /// @details Computed as the sum of half the KCL width and an optional additional length
    f32 getCollisionRadius() const override {
        return m_bboxHalfSideLength + colRadiusAdditionalLength();
    }

    void initCollision() override;

    /// @addr{0x806810F8}
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointPartial(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x806811B0}
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointPartialPush(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80681268}
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointFull(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80681320}
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointFullPush(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680DF4}
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        calcScale(timeOffset);
        update(timeOffset);

        return m_objColMgr->checkSpherePartial(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680EF0}
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        calcScale(timeOffset);
        update(timeOffset);

        return m_objColMgr->checkSpherePartialPush(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680FEC}
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        calcScale(timeOffset);
        update(timeOffset);

        return m_objColMgr->checkSphereFull(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x806810E8}
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkCollision(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x806807E8}
    void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 /*timeOffset*/) override {
        m_objColMgr->narrScLocal(radius, pos, mask);
    }

    /// @addr{0x80680B14}
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointCachedPartial(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680BCC}
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointCachedPartialPush(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680C84}
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointCachedFull(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680D3C}
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override {
        return m_objColMgr->checkPointCachedFullPush(pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x806807F0}
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedPartial(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x806808FC}
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedPartialPush(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680A08}
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedFull(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80680B04}
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkCollisionCached(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    virtual void update(u32 timeOffset);
    virtual void calcScale(u32 timeOffset);

    /// @brief Sets the velocity that describes the movement effect when a player is on the object
    /// @addr{0x80681450}
    virtual void setMovingObjVel(const EGG::Vector3f &v) {
        m_objColMgr->setMovingObjVel(v);
    }

    /// @brief Computes the collision transformation matrix for the current frame. The return value
    /// is assigned to the object collision manager.
    /// @addr{0x807FEAC0}
    [[nodiscard]] virtual const EGG::Matrix34f &getUpdatedMatrix(u32 /*timeOffset*/) {
        calcTransform();
        return transform();
    }

    /// @brief Updates the collision manager's scale for the current frame
    /// @addr{0x80687DB0}
    [[nodiscard]] virtual f32 getScaleY(u32 /* timeOffset */) const {
        return scale().y;
    }

    /// @brief Optional additional length to add when computing the collision radius
    /// @addr{0x8068143C}
    [[nodiscard]] virtual f32 colRadiusAdditionalLength() const {
        return 0.0f;
    }

    /// @brief Checks collision between a sphere and the object, writing out full collision info
    /// @desync This function can result in physics desynchronizations when racing a ghost.
    /// Since this function is only called when the player/ghost's hitbox is close enough to the
    /// object (as per GJK collision checks), it is possible that the first player hitbox is just
    /// barely too far away from the object while the second player hitbox is just close enough to
    /// pass the GJK check and thus call this function and update the collision manager's transform.
    /// Subsequently, if the ghost's first hitbox normally would be too far away from the object to
    /// pass the GJK check, the transform update from the player's second hitbox check means that
    /// the ghost's first hitbox is now being checked against a different transform than the player,
    /// which can result in the GJK check passing when it originally would fail. This is the reason
    /// that the DS Delfino Square bridge can sometimes desync.
    /// @addr{0x8081AFB4}
    [[nodiscard]] virtual bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @brief Checks collision between a sphere and the object, using only cached KCL prisms,
    /// writing out full collision info
    /// @desync This function can result in physics desynchronizations when racing a ghost.
    /// Since this function is only called when the player/ghost's hitbox is close enough to the
    /// object (as per GJK collision checks), it is possible that the first player hitbox is just
    /// barely too far away from the object while the second player hitbox is just close enough to
    /// pass the GJK check and thus call this function and update the collision manager's transform.
    /// Subsequently, if the ghost's first hitbox normally would be too far away from the object to
    /// pass the GJK check, the transform update from the player's second hitbox check means that
    /// the ghost's first hitbox is now being checked against a different transform than the player,
    /// which can result in the GJK check passing when it originally would fail. This is the reason
    /// that the DS Delfino Square bridge can sometimes desync.
    /// @addr{0x8081B16C}
    [[nodiscard]] virtual bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

protected:
    ObjColMgr *m_objColMgr;      ///< Collision manager for the object's KCL data
    EGG::Vector3f m_kclMidpoint; ///< The midpoint of the KCL's bounding box
    f32 m_bboxHalfSideLength;    ///< Half of the KCL's bounding box width

    /// @brief Frame of most recent collision transform update.
    /// @details Used to avoid redundant matrix calculations within the same frame.
    s32 m_lastMtxUpdateFrame;

    /// @brief Frame of most recent scale update.
    /// @details Used to keep the scale frame in sync with the matrix update frame.
    s32 m_lastScaleUpdateFrame;
};

} // namespace Kinoko::Field
