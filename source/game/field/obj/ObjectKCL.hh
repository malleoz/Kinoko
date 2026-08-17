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
    void calcCollisionTransform() override;

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
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 timeOffset) override;
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

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
    [[nodiscard]] virtual bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

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
    [[nodiscard]] virtual bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

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
