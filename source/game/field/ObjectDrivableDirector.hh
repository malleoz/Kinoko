#pragma once

#include "game/field/KColData.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

class MapdataGeoObj;

} // namespace System

namespace Field {

class ObjectDrivable;
class ObjectObakeManager;

/// @brief Singleton that manages all @ref ObjectDrivable instances and their collision interactions
/// @details Distinguishes between objects that require per-frame calculations and those that do
/// not. Exposes multiple interfaces to perform collision queries similar to that of @ref
/// CourseColMgr, which call into each object's respective virtual collision check method.
/// Separately owns the @ref ObjectObakeManager, which handles SNES Ghost Valley 2 block
/// management and spatial indexing for drivable objects.
class ObjectDrivableDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    void init();
    void calc();

    void addObject(ObjectDrivable *obj);
    void createObakeManager(const System::MapdataGeoObj &params);

    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

    void colNarScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask, u32 timeOffset);

    /// @beginGetters

    [[nodiscard]] ObjectObakeManager *obakeManager() const;

    /// @endGetters

    static ObjectDrivableDirector *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static ObjectDrivableDirector *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    ObjectDrivableDirector();
    ~ObjectDrivableDirector() override;

    /// @brief Function pointer type for an @ref ObjectDrivable sphere collision check
    /// @tparam T The collision info object type, either @ref CollisionInfo or @ref
    /// CollisionInfoPartial
    template <typename T>
    using DrivableCheckFunc = bool (ObjectDrivable::*)(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset, DrivableCheckFunc<T> checkFunc);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereCachedImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset, DrivableCheckFunc<T> cachedCheckFunc, DrivableCheckFunc<T> checkFunc);

    fixed_vector<ObjectDrivable *> m_objects;     ///< All drivable objects live here
    fixed_vector<ObjectDrivable *> m_calcObjects; ///< Objects needing calc() live here too
    ObjectObakeManager *m_obakeManager;           ///< Manages rGV2 blocks and spatial indexing

    static constexpr size_t MAX_OBJECTS = 400; ///< Maximum number of objects in the vectors

    /// @addr{0x809C4310}
    /// @brief The pointer to the singleton instance of the class
    static ObjectDrivableDirector *s_instance;
};

} // namespace Field

} // namespace Kinoko
