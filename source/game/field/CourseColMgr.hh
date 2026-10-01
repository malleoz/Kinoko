#pragma once

#include "game/field/KColData.hh"

#include "game/system/ResourceManager.hh"

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
/// for both a point and a sphere. Some queries will cache the result in the @ref CollisionDirector
/// collision entry cache. This class also stores a KCL scale factor that is passed into queries so
/// that the object collision queries that are forwarded to @ref CourseColMgr can compensate for
/// dynamically sized objects (such as the Bowser's Castle geysers, represented by @ref
/// ObjectFlamePoleFoot).
class CourseColMgr : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

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
    };
    STATIC_ASSERT(sizeof(NoBounceWallColInfo) == 0x34);

    /// @addr{0x807C28D8}
    /// @brief Parses and caches tris stored in `course.kcl`
    /// @details In the base game, this file is loaded in @ref CollisionDirector::CreateInstance()
    /// and passed into this function. It's simpler to just keep it here.
    void init() {
        std::span<const u8> file = LoadFile("course.kcl");
        m_data = EGG::egg_new<KColData>(file.data());
    }

    /// @addr{0x807C293C}
    /// @brief Narrows the spatial cache in @ref KColData to only include course KCL tris defined by
    /// the provided mask within a certain radius of the given position.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check within
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point of the sphere to check within
    /// @param mask The KCL flags to check collision against (other types are ignored)
    void scaledNarrowScopeLocal(f32 scale, f32 radius, KColData *data, const EGG::Vector3f &pos,
            KCLTypeMask mask) {
        if (!data) {
            data = m_data;
        }

        f32 invScale = 1.0f / scale;
        data->narrowScopeLocal(pos * invScale, radius * invScale, mask);
    }

    /// @addr{0x807C2A60}
    /// @brief Checks collision between a point and course KCL tris, writing only partial collision
    /// info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, false, false);
    }

    /// @addr{0x807C2DA0}
    /// @brief Checks collision between a point and course KCL tris, writing only partial collision
    /// info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointPartialPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, false, true);
    }

    /// @addr{0x807C30E0}
    /// @brief Checks collision between a point and course KCL tris, writing out full collision info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, false, false);
    }

    /// @addr{0x807C3554}
    /// @brief Checks collision between a point and course KCL tris, writing out full collision
    /// info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, false, true);
    }

    /// @addr{0x807C39C8}
    /// @brief Checks collision between a sphere and course KCL tris, writing partial collision info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSpherePartial(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, false,
                false);
    }

    /// @addr{0x807C3B5C}
    /// @brief Checks collision between a sphere and course KCL tris, writing partial collision
    /// info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSpherePartialPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, false, true);
    }

    /// @addr{0x807C3CF0}
    /// @brief Checks collision between a sphere and course KCL tris, writing full collision info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFull(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, false,
                false);
    }

    /// @addr{0x807C3E84}
    /// @brief Checks collision between a sphere and course KCL tris, writing full collision info.
    /// Additionally pushes the collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFullPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, false, true);
    }

    /// @addr{0x807C4018}
    /// @brief Checks collision between a point and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing only partial collision info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointCachedPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, true, false);
    }

    /// @addr{0x807C41A4}
    /// @brief Checks collision between a point and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing only partial collision info. Additionally
    /// pushes the collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointCachedPartialPush(f32 scale, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, true, true);
    }

    /// @addr{0x807C4330}
    /// @brief Checks collision between a point and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing out full collision info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointCachedFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, true, false);
    }

    /// @addr{0x807C44BC}
    /// @brief Checks collision between a point and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing out full collision info. Additionally pushes
    /// the collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkPointCachedFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) {
        return checkPointImpl(data, scale, pos, prevPos, mask, info, maskOut, true, true);
    }

    /// @addr{0x807C4648}
    /// @brief Checks collision between a sphere and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing partial collision info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedPartial(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, true, false);
    }

    /// @addr{0x807C47F0}
    /// @brief Checks collision between a sphere and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing partial collision info. Additionally pushes
    /// the collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfoPartial *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, true, true);
    }

    /// @addr{0x807C4998}
    /// @brief Checks collision between a sphere and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing full collision info
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedFull(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, true, false);
    }

    /// @addr{0x807C4B40}
    /// @brief Checks collision between a sphere and course KCL tris by using the @ref
    /// CollisionDirector local spatial cache, writing full collision info. Additionally pushes the
    /// collision entry into the @ref CollisionDirector cache.
    /// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
    /// @param radius The radius of the sphere to check
    /// @param data Pointer to the parsed tri data to perform the lookup on
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedFullPush(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CollisionInfo *info, KCLTypeMask *maskOut) {
        return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, true, true);
    }

    /// @beginSetters

    /// @brief Sets the @ref NoBounceWallColInfo struct that should be used to accumulate soft
    /// wall collision info on subsequent queries
    /// @param info Pointer to the @ref NoBounceWallColInfo struct to be used for accumulating soft
    /// wall collision info
    void setNoBounceWallInfo(NoBounceWallColInfo *info) {
        m_noBounceWallInfo = info;
    }

    /// @brief Clears the @ref NoBounceWallColInfo pointer so collision queries do not bother
    /// accumulating soft wall info
    void clearNoBounceWallInfo() {
        m_noBounceWallInfo = nullptr;
    }

    /// @brief Sets the local-to-world transformation matrix that is used to map object collisions
    /// from local space to world space before accumulating soft wall collision data into @ref
    /// NoBounceWallColInfo
    /// @param mtx Pointer to the local-to-world transformation matrix to be used for mapping object
    /// collisions from local space to world space
    /// @details Main course KCL collisions are already based in world space, whereas collisions
    /// queried via @ref ObjColMgr exist within the object's local space.
    void setLocalMtx(EGG::Matrix34f *mtx) {
        m_localMtx = mtx;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Gets a pointer to the @ref KColData that represents the course's KCL
    /// @return A const pointer to the course's @ref KColData
    [[nodiscard]] const KColData *data() const {
        return m_data;
    }

    /// @brief Gets a pointer to the @ref NoBounceWallColInfo struct used for accumulating soft wall
    /// collision info
    /// @return A pointer to the @ref NoBounceWallColInfo struct
    [[nodiscard]] NoBounceWallColInfo *noBounceWallInfo() const {
        return m_noBounceWallInfo;
    }

    /// @endGetters

    /// @brief Loads the provided course file from the resource manager
    /// @param filename The name of the course file to load
    /// @return A span containing the data of the loaded course file
    [[nodiscard]] static std::span<const u8> LoadFile(const char *filename) {
        auto *resMgr = System::ResourceManager::Instance();
        return resMgr->getFile(filename, System::ArchiveId::Course);
    }

    /// @addr{0x807C2824}
    /// @brief Creates a singleton instance of the @ref CourseColMgr
    /// @return A pointer to the newly created @ref CourseColMgr instance
    static CourseColMgr *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<CourseColMgr>();
        return s_instance;
    }

    /// @addr{0x807C2884}
    /// @brief Destroys the singleton instance of the @ref CourseColMgr
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Gets the singleton instance of the @ref CourseColMgr
    /// @return A pointer to the singleton instance of the @ref CourseColMgr
    [[nodiscard]] static CourseColMgr *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    /// @addr{0x807C29E4}
    /// @brief Private constructor
    /// @details Initializes @ref m_data, @ref m_noBounceWallInfo, and @ref m_localMtx to nullptr,
    /// and sets @ref m_kclScale to 1.0f.
    CourseColMgr()
        : m_data(nullptr),
          m_kclScale(1.0f),
          m_noBounceWallInfo(nullptr),
          m_localMtx(nullptr) {}

    /// @addr{0x807C2A04}
    /// @brief Private virtual destructor that also destroys the underlying @ref KColData
    ~CourseColMgr() override {
        if (s_instance) {
            s_instance = nullptr;
            WARN("CourseColMgr instance not explicitly handled!");
        }

        ASSERT(m_data);
        EGG::egg_delete(m_data);
    }

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
