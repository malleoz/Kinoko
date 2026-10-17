#pragma once

#include "game/field/KCollisionTypes.hh"

#include <egg/math/Matrix.hh>

namespace Kinoko::Field {

struct CollisionInfo;
struct CollisionInfoPartial;
class CourseColMgr;
class KColData;

/// @brief Manager for an object's KCL interactions.
/// @details Exposes collision queries which operate in local space rather than world space, in
/// order to allow for dynamically sized objects. Parses tris from an object's KCL file so that
/// collision queries can be performed against the object's KCL tris. Queries can be performed for
/// both a point and a sphere. Some queries will cache the result in the @ref CollisionDirector
/// collision entry cache. This class also stores a KCL scale factor so that the object collision
/// queries can map local space collision info to world space in order to compensate for dynamically
/// sized objects (such as the Bowser's Castle geysers, represented by @ref ObjectFlamePoleFoot).
class ObjColMgr {
public:
    ObjColMgr(const void *file);
    ~ObjColMgr();

    void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask);

    [[nodiscard]] EGG::Vector3f kclLowWorld() const;
    [[nodiscard]] EGG::Vector3f kclHighWorld() const;

    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    /// @beginSetters

    void setMtx(const EGG::Matrix34f &mtx);
    void setInvMtx(const EGG::Matrix34f &mtx);
    void setScale(f32 val);
    void setMovingObjVel(const EGG::Vector3f &v);

    /// @endSetters

private:
    /// @brief Function pointer type for a @ref CourseColMgr point collision check
    /// @tparam T The collision info object type, either @ref CollisionInfo or @ref
    /// CollisionInfoPartial
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The current position of the point being checked
    /// @param prevPos The previous position of the point being checked
    /// @param mask The KCL flags to include in the collision check
    /// @param info The collision info object to populate with the results
    /// @param maskOut Optional pointer to store the resulting KCL flags after the check
    /// @return `true` if a collision was detected, `false` otherwise
    template <typename T>
    using CourseColCheckPointFunc = bool (CourseColMgr::*)(f32 scale, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
            KCLTypeMask *maskOut);

    /// @brief Function pointer type for a @ref CourseColMgr sphere collision check
    /// @tparam T The collision info object type, either @ref CollisionInfo or @ref
    /// CollisionInfoPartial
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere being checked
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The current position of the sphere being checked
    /// @param prevPos The previous position of the sphere being checked
    /// @param mask The KCL flags to include in the collision check
    /// @param info The collision info object to populate with the results
    /// @param maskOut Optional pointer to store the resulting KCL flags after the check
    /// @return `true` if a collision was detected, `false` otherwise
    template <typename T>
    using CourseColCheckSphereFunc = bool (CourseColMgr::*)(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
            KCLTypeMask *maskOut);

    template <typename T>
    [[nodiscard]] bool checkPointImpl(CourseColCheckPointFunc<T> fn, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut);

    template <typename T>
    [[nodiscard]] bool checkSphereImpl(CourseColCheckSphereFunc<T> fn, f32 radius,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
            KCLTypeMask *maskOut);

    void mergeInfo(CollisionInfoPartial &info, CollisionInfoPartial &tempInfo) const;
    void mergeInfo(CollisionInfo &info, CollisionInfo &tempInfo) const;

    KColData *const m_data;       ///< Pointer to parsed KCL tri data
    EGG::Matrix34f m_mtx;         ///< The local-to-world transformation matrix
    EGG::Matrix34f m_mtxInv;      ///< The world-to-local transformation matrix
    f32 m_kclScale;               ///< Scale factor for collision queries
    EGG::Vector3f m_movingObjVel; ///< The object's velocity
};

} // namespace Kinoko::Field
