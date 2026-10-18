#pragma once

#include "game/field/KColData.hh"

// Credit: em-eight/mkw

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

class KColData;

/// @brief Function type that represents a particular KColData collision query
/// @details In practice, this is one of @ref KColData::checkPointCollision or @ref
/// KColData::checkSphereCollision.
typedef bool (
        KColData::*CollisionCheckFunc)(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut);

/// @brief Manager for course KCL interactions
/// @details Exposes multiple interfaces to query for collision checks. Parses tris from course.kcl
/// so that collision queries can be performed against the course KCL tris. Queries can be performed
/// for both a point and a sphere. Some queries will cache the result in the @ref CollisionDirector
/// collision entry cache. This class also stores a KCL scale factor that is passed into queries so
/// that the object collision queries that are forwarded to @ref CourseColMgr can compensate for
/// dynamically sized objects (such as the Bowser's Castle geysers, represented by @ref
/// ObjectFlamePoleFoot).
class CourseColMgr : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    /// @brief Collision info pertaining to soft walls
    /// @details Stored as a pointer member of @ref CourseColMgr so that @ref Kart::KartCollide can
    /// query collision and fetch soft wall information to push the player out of the soft wall
    /// geometry while still letting @ref CourseColMgr separately track normal wall hits.
    struct NoBounceWallColInfo {
        EGG::BoundBox3f bbox;     ///< Bounding box of "push out" vectors
        EGG::Vector3f tangentOff; ///< The net "push out" vector
        f32 dist;                 ///< Depth of collision into the tri
        EGG::Vector3f fnrm;       ///< Face normal of the colliding tri

        /// @brief Expands the bounding box to include the provided position vector
        /// @param offset The position offset to include in the bounding box
        void updateBBox(const EGG::Vector3f &offset) {
            bbox.min = bbox.min.minimize(offset);
            bbox.max = bbox.max.maximize(offset);
        }
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
    [[nodiscard]] bool checkPointFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
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
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFull(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut);

    /// @beginSetters

    void setNoBounceWallInfo(NoBounceWallColInfo *info);
    void clearNoBounceWallInfo();
    void setLocalMtx(EGG::Matrix34f *mtx);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] const KColData *data() const;
    [[nodiscard]] NoBounceWallColInfo *noBounceWallInfo() const;

    /// @endGetters

    [[nodiscard]] static std::span<const u8> LoadFile(const char *filename);

    static CourseColMgr *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static CourseColMgr *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    CourseColMgr();
    ~CourseColMgr() override;

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkPointImpl(KColData *data, f32 scale, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            bool cached, bool push);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(KColData *data, f32 scale, f32 radius,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
            KCLTypeMask *maskOut, bool cached, bool push);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool doCheckWithInfo(KColData *data, CollisionCheckFunc collisionCheckFunc,
            T *info, KCLTypeMask *maskOut, bool push);
    [[nodiscard]] bool doCheckMaskOnly(KColData *data, CollisionCheckFunc collisionCheckFunc,
            KCLTypeMask *maskOut, bool push);

    KColData *m_data;                        ///< Pointer to he parsed tri data from course.kcl
    f32 m_kclScale;                          ///< Scale factor for collision queries
    NoBounceWallColInfo *m_noBounceWallInfo; ///< Accumulates soft wall collision info
    EGG::Matrix34f *m_localMtx; ///< Local-to-world transformation matrix for object tris

    /// @addr{0x809C3C10}
    /// @brief The pointer to the singleton instance of the class
    static CourseColMgr *s_instance;
};

} // namespace Field

} // namespace Kinoko
