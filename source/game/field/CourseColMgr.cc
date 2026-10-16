#include "CourseColMgr.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/ResourceManager.hh"

// Credit: em-eight/mkw

namespace Kinoko::Field {

/// @addr{0x807C28D8}
/// @brief Parses and caches tris stored in `course.kcl`
/// @details In the base game, this file is loaded in @ref CollisionDirector::CreateInstance()
/// and passed into this function. It's simpler to just keep it here.
void CourseColMgr::init() {
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
void CourseColMgr::scaledNarrowScopeLocal(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, KCLTypeMask mask) {
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
bool CourseColMgr::checkPointPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
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
bool CourseColMgr::checkPointPartialPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
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
bool CourseColMgr::checkPointFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
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
bool CourseColMgr::checkPointFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
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
bool CourseColMgr::checkSpherePartial(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, false, false);
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
bool CourseColMgr::checkSpherePartialPush(f32 scale, f32 radius, KColData *data,
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
bool CourseColMgr::checkSphereFull(f32 scale, f32 radius, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, false, false);
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
bool CourseColMgr::checkSphereFullPush(f32 scale, f32 radius, KColData *data,
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
bool CourseColMgr::checkPointCachedPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
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
bool CourseColMgr::checkPointCachedPartialPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
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
bool CourseColMgr::checkPointCachedFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
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
bool CourseColMgr::checkPointCachedFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
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
bool CourseColMgr::checkSphereCachedPartial(f32 scale, f32 radius, KColData *data,
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
bool CourseColMgr::checkSphereCachedPartialPush(f32 scale, f32 radius, KColData *data,
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
bool CourseColMgr::checkSphereCachedFull(f32 scale, f32 radius, KColData *data,
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
bool CourseColMgr::checkSphereCachedFullPush(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereImpl(data, scale, radius, pos, prevPos, mask, info, maskOut, true, true);
}

/// @brief Sets the @ref NoBounceWallColInfo struct that should be used to accumulate soft
/// wall collision info on subsequent queries
/// @param info Pointer to the @ref NoBounceWallColInfo struct to be used for accumulating soft
/// wall collision info
void CourseColMgr::setNoBounceWallInfo(NoBounceWallColInfo *info) {
    m_noBounceWallInfo = info;
}

/// @brief Clears the @ref NoBounceWallColInfo pointer so collision queries do not bother
/// accumulating soft wall info
void CourseColMgr::clearNoBounceWallInfo() {
    m_noBounceWallInfo = nullptr;
}

/// @brief Sets the local-to-world transformation matrix that is used to map object collisions
/// from local space to world space before accumulating soft wall collision data into @ref
/// NoBounceWallColInfo
/// @param mtx Pointer to the local-to-world transformation matrix to be used for mapping object
/// collisions from local space to world space
/// @details Main course KCL collisions are already based in world space, whereas collisions
/// queried via @ref ObjColMgr exist within the object's local space.
void CourseColMgr::setLocalMtx(EGG::Matrix34f *mtx) {
    m_localMtx = mtx;
}

/// @brief Gets a pointer to the @ref KColData that represents the course's KCL
/// @return A const pointer to @ref m_data.
const KColData *CourseColMgr::data() const {
    return m_data;
}

/// @brief Gets a pointer to the @ref NoBounceWallColInfo struct used for accumulating soft wall
/// collision info
/// @return A pointer to @ref m_noBounceWallInfo.
CourseColMgr::NoBounceWallColInfo *CourseColMgr::noBounceWallInfo() const {
    return m_noBounceWallInfo;
}

/// @brief Loads the provided course file from the resource manager
/// @param filename The name of the course file to load
/// @return A span containing the data of the loaded course file
std::span<const u8> CourseColMgr::LoadFile(const char *filename) {
    auto *resMgr = System::ResourceManager::Instance();
    return resMgr->getFile(filename, System::ArchiveId::Course);
}

/// @addr{0x807C2824}
/// @brief Creates a singleton instance of the @ref CourseColMgr
/// @return A pointer to the newly created @ref CourseColMgr instance
/// @pre Assumes that no singleton instance of @ref CourseColMgr has been created yet.
CourseColMgr *CourseColMgr::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<CourseColMgr>();
    return s_instance;
}

/// @addr{0x807C2884}
/// @brief Destroys the singleton instance of the @ref CourseColMgr
/// @pre Assumes that a singleton instance of @ref CourseColMgr has already been created.
void CourseColMgr::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Gets the singleton instance of the @ref CourseColMgr
/// @return A pointer to the singleton instance of the @ref CourseColMgr
CourseColMgr *CourseColMgr::Instance() {
    return s_instance;
}

/// @addr{0x807C29E4}
/// @brief Private constructor
/// @details Initializes @ref m_data, @ref m_noBounceWallInfo, and @ref m_localMtx to nullptr,
/// and sets @ref m_kclScale to 1.0f.
CourseColMgr::CourseColMgr()
    : m_data(nullptr),
      m_kclScale(1.0f),
      m_noBounceWallInfo(nullptr),
      m_localMtx(nullptr) {}

/// @addr{0x807C2A04}
/// @brief Private virtual destructor that also destroys the underlying @ref KColData
CourseColMgr::~CourseColMgr() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("CourseColMgr instance not explicitly handled!");
    }

    ASSERT(m_data);
    EGG::egg_delete(m_data);
}

/// @brief Helper function which contains frequently re-used code for the checkPoint* family of
/// collision check functions
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param cached Whether the check should use the local spatial cache
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details If `data` is `nullptr`, the function will use the internal course collision data
/// (`m_data`). If `cached` is true and the local spatial cache is empty, the function will
/// early-return `false`. Sets @ref m_kclScale to the provided `scale` value. Dispatches to @ref
/// KColData::lookupPoint() to prepare for the collision check. Calls the appropriate `doCheck*`
/// function based on whether `info` is `nullptr`.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool CourseColMgr::checkPointImpl(KColData *data, f32 scale, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut, bool cached,
        bool push) {
    if (!data) {
        data = m_data;
    }

    if (cached && data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevPos * invScale, mask);

    if (info) {
        return doCheckWithInfo(data, &KColData::checkPointCollision, info, maskOut, push);
    }

    return doCheckMaskOnly(data, &KColData::checkPointCollision, maskOut, push);
}

/// @brief Shared implementation backing the checkSphere* family of functions
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param cached Whether the check should use the local spatial cache
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details If `data` is `nullptr`, the function will use the internal course collision data
/// (`m_data`). If `cached` is true and the local spatial cache is empty, the function will
/// early-return `false`. Sets @ref m_kclScale to the provided `scale` value. Dispatches to @ref
/// @ref KColData::lookupSphere() or @ref KColData::lookupSphereCached() depending on whether
/// `cached` is `true` to prepare for the collision check. Calls the appropriate `doCheck*` function
/// based on whether `info` is `nullptr`.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool CourseColMgr::checkSphereImpl(KColData *data, f32 scale, f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut, bool cached,
        bool push) {
    if (!data) {
        data = m_data;
    }

    if (cached && data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    if (cached) {
        data->lookupSphereCached(radius * invScale, pos * invScale, prevPos * invScale, mask);
    } else {
        data->lookupSphere(radius * invScale, pos * invScale, prevPos * invScale, mask);
    }

    if (info) {
        return doCheckWithInfo(data, &KColData::checkSphereCollision, info, maskOut, push);
    }

    return doCheckMaskOnly(data, &KColData::checkSphereCollision, maskOut, push);
}

/// @addr{0x807C2BD8}
/// @brief Calls into the provided KColData query function, accumulating collision info for each
/// collision
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details This function repeatedly calls the provided @ref KColData query function until no more
/// collisions are detected. For each collision, scales the collision distance by `m_kclScale` to
/// account for local-to-world transformations.
/// If @ref m_noBounceWallInfo is set and the collision is a soft wall, it updates the soft wall
/// collision info accordingly.
///
/// Otherwise, if `maskOut` is provided, it sets the @ref KColType bits that were hit; it also
/// pushes to the @ref CollisionDirector cache if `push` is `true`. If the colliding flags include
/// #KCL_TYPE_SOLID_SURFACE, `info` is updated accordingly.
///
/// Finally, clears @ref m_localMtx to ensure it does not affect subsequent collision checks.
/// Returns `true` if at least one collision occurred.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool CourseColMgr::doCheckWithInfo(KColData *data, CollisionCheckFunc collisionCheckFunc, T *info,
        KCLTypeMask *maskOut, bool push) {
    f32 dist;
    EGG::Vector3f fnrm;
    u16 attribute;
    bool hasCol = false;

    while ((data->*collisionCheckFunc)(&dist, &fnrm, &attribute)) {
        hasCol = true;
        dist *= m_kclScale;

        if (m_noBounceWallInfo && (attribute & KCL_SOFT_WALL_MASK)) {
            if (m_localMtx) {
                fnrm = m_localMtx->multVector33(fnrm);
            }
            EGG::Vector3f offset = fnrm * dist;
            m_noBounceWallInfo->updateBBox(offset);
            if (m_noBounceWallInfo->dist < dist) {
                m_noBounceWallInfo->dist = dist;
                m_noBounceWallInfo->fnrm = fnrm;
            }
        } else {
            u32 flags = KCL_ATTRIBUTE_TYPE_BIT(attribute);
            if (maskOut) {
                if (push) {
                    CollisionDirector::Instance()->pushCollisionEntry(dist, maskOut, flags,
                            attribute);
                } else {
                    *maskOut = *maskOut | flags;
                }
            }
            if (flags & KCL_TYPE_SOLID_SURFACE) {
                if constexpr (std::is_same_v<T, CollisionInfoPartial>) {
                    EGG::Vector3f offset = fnrm * dist;
                    info->bbox.min = info->bbox.min.minimize(offset);
                    info->bbox.max = info->bbox.max.maximize(offset);
                } else {
                    info->update(dist, fnrm * dist, fnrm, flags);
                }
            }
        }
    }

    m_localMtx = nullptr;

    return hasCol;
}

/// @brief Calls into the provided @ref KColData query function, accumulating the colliding base
/// type flags and conditionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details This function repeatedly calls the provided `collisionCheckFunc` on the provided @ref
/// KColData object. For each collision, it accumulates the colliding @ref KColType flags, and
/// if `push` is `true`, also pushes the collision entry into the @ref CollisionDirector cache. This
/// function skips soft wall collisions if @ref m_noBounceWallInfo is set. Returns `true` if at
/// least one collision occurred.
bool CourseColMgr::doCheckMaskOnly(KColData *data, CollisionCheckFunc collisionCheckFunc,
        KCLTypeMask *maskOut, bool push) {
    bool hasCol = false;
    f32 dist;
    u16 attribute;

    while ((data->*collisionCheckFunc)(&dist, nullptr, &attribute)) {
        if ((!m_noBounceWallInfo || !(attribute & KCL_SOFT_WALL_MASK)) && maskOut) {
            if (push) {
                CollisionDirector::Instance()->pushCollisionEntry(dist, maskOut,
                        KCL_ATTRIBUTE_TYPE_BIT(attribute), attribute);
            } else {
                *maskOut |= KCL_ATTRIBUTE_TYPE_BIT(attribute);
            }
        }
        hasCol = true;
    }

    return hasCol;
}

CourseColMgr *CourseColMgr::s_instance = nullptr;

} // namespace Kinoko::Field
