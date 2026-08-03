#pragma once

#include "game/field/KColData.hh"

#include <egg/math/BoundBox.hh>
#include <egg/math/Matrix.hh>

// Credit: em-eight/mkw

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

/// @brief Function type that represents a particular KColData collision query
/// @details In practice, this is one of @ref KColData::checkPointCollision or @ref
/// KColData::checkSphereCollision.
typedef bool (
        KColData::*CollisionCheckFunc)(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut);

/// @brief Manager for course KCL interactions
/// @details Exposes multiple interfaces to query for collision checks. Parses tris from course.kcl
/// so that collision queries can be performed against the course KCL tris. Queries can be performed
/// for both a point and a sphere. Some queries will cache the result in the @ref
/// CollisionDirector's collision entry cache. This class also stores a KCL scale factor that is
/// passed into queries so that object collision queries that are forwarded to @ref CourseColMgr can
/// compensate for dynamically sized objects (such as the Bowser's Castle geysers, represented by
/// @ref ObjectFlamePoleFoot).
class CourseColMgr : EGG::Disposer {
    friend class Host::Context;

public:
    /// @brief Collision info pertaining to soft walls
    /// @details Stored as a pointer member of @ref CourseColMgr so that `KartCollide` can query
    /// collision and fetch soft wall information to push the player out of the soft wall geometry
    /// while still letting @ref CourseColMgr separately track normal wall hits.
    struct NoBounceWallColInfo {
        EGG::BoundBox3f bbox;     ///< Bounding box of "push out" vectors
        EGG::Vector3f tangentOff; ///< The net "push out" vector
        f32 dist;                 ///< Depth of collision into the tri
        EGG::Vector3f fnrm;       ///< Face normal of the colliding tri
    };
    STATIC_ASSERT(sizeof(NoBounceWallColInfo) == 0x34);

    void init();

    void scaledNarrowScopeLocal(f32 scale, f32 radius, KColData *data, const EGG::Vector3f &pos,
            KCLTypeMask mask);

    [[nodiscard]] bool checkPointPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointPartialPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFull(f32 kclScale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFullPush(f32 kclScale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSpherePartial(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSpherePartialPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFull(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFullPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut);

    [[nodiscard]] bool checkPointCachedPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedPartialPush(f32 scale, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSphereCachedPartial(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask typeMask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask typeMask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFull(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask typeMask,
            CollisionInfo *colInfo, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask typeMask,
            CollisionInfo *colInfo, KCLTypeMask *maskOut);

    /// @beginSetters
    /// @brief Points to the @ref NoBounceWallColInfo struct that should be used to accumulate soft
    /// wall collision info on subsequent queries
    void setNoBounceWallInfo(NoBounceWallColInfo *info) {
        m_noBounceWallInfo = info;
    }

    /// @brief Removes the @ref NoBounceWallColInfo pointer so collision queries do not bother
    /// accumulating soft wall info
    void clearNoBounceWallInfo() {
        m_noBounceWallInfo = nullptr;
    }

    /// @brief Sets the local-to-world transformation matrix that is used to map object collisions
    /// from local space to world space before accumulating soft wall collision data into @ref
    /// NoBounceWallColInfo
    /// @details Main course KCL collisions are already based in world space, whereas collisions
    /// queried via @ref ObjColMgr exist within the object's local space.
    void setLocalMtx(EGG::Matrix34f *mtx) {
        m_localMtx = mtx;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] const KColData *data() const {
        return m_data;
    }

    [[nodiscard]] NoBounceWallColInfo *noBounceWallInfo() const {
        return m_noBounceWallInfo;
    }
    /// @endGetters

    static void *LoadFile(const char *filename);

    static CourseColMgr *CreateInstance();
    static void DestroyInstance();

    [[nodiscard]] static CourseColMgr *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    CourseColMgr();
    ~CourseColMgr() override;

    [[nodiscard]] bool doCheckWithPartialInfo(KColData *data, CollisionCheckFunc collisionCheckFunc,
            CollisionInfoPartial *info, KCLTypeMask *typeMask);
    [[nodiscard]] bool doCheckWithPartialInfoPush(KColData *data,
            CollisionCheckFunc collisionCheckFunc, CollisionInfoPartial *info,
            KCLTypeMask *typeMask);
    [[nodiscard]] bool doCheckWithFullInfo(KColData *data, CollisionCheckFunc collisionCheckFunc,
            CollisionInfo *colInfo, KCLTypeMask *flagsOut);
    [[nodiscard]] bool doCheckWithFullInfoPush(KColData *data,
            CollisionCheckFunc collisionCheckFunc, CollisionInfo *colInfo, KCLTypeMask *flagsOut);
    [[nodiscard]] bool doCheckMaskOnly(KColData *data, CollisionCheckFunc collisionCheckFunc,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool doCheckMaskOnlyPush(KColData *data, CollisionCheckFunc collisionCheckFunc,
            KCLTypeMask *maskOut);

    KColData *m_data;                        ///< Pointer to he parsed tri data from course.kcl
    f32 m_kclScale;                          ///< Scale factor for collision queries
    NoBounceWallColInfo *m_noBounceWallInfo; ///< Accumulates soft wall collision info
    EGG::Matrix34f *m_localMtx; ///< Local-to-world transformation matrix for object tris

    static CourseColMgr *s_instance; ///< @addr{0x809C3C10}
};

} // namespace Field

} // namespace Kinoko
