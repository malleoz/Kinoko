#pragma once

#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectDrivable.hh"
#include "game/field/obj/ObjectObakeManager.hh"

#include <egg/core/Allocator.hh>

#include <vector>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

/// @brief Singleton that manages all @ref ObjectDrivable instances and their collision interactions
/// @details Distinguishes between objects that require per-frame calculations and those that do
/// not. Exposes multiple interfaces to perform collision queries similar to that of @ref
/// CourseColMgr, which call into each object's respective virtual collision check method.
/// Separately owns the @ref ObjectObakeManager, which handles SNES Ghost Valley 2 block
/// management and spatial indexing for drivable objects.
class ObjectDrivableDirector : EGG::Disposer {
    friend class Host::Context;

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

    [[nodiscard]] ObjectObakeManager *obakeManager() const {
        return m_obakeManager;
    }

    /// @addr{0x8081B428}
    /// @brief Creates the singleton instance of @ref ObjectDrivableDirector
    /// @return A pointer to the newly created singleton instance of @ref ObjectDrivableDirector
    static ObjectDrivableDirector *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<ObjectDrivableDirector>();
        return s_instance;
    }

    /// @addr{0x8081B4B0}
    /// @brief Destroys the singleton instance of the @ref ObjectDrivableDirector
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref ObjectDrivableDirector
    /// @return A pointer to the singleton instance of the @ref ObjectDrivableDirector
    [[nodiscard]] static ObjectDrivableDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    ObjectDrivableDirector();
    ~ObjectDrivableDirector() override;

    fixed_vector<ObjectDrivable *> m_objects;     ///< All drivable objects live here
    fixed_vector<ObjectDrivable *> m_calcObjects; ///< Objects needing calc() live here too
    ObjectObakeManager *m_obakeManager;           ///< Manages rGV2 blocks and spatial indexing

    static constexpr size_t MAX_OBJECTS = 400; ///< Maximum number of objects in the vectors

    static ObjectDrivableDirector *s_instance; ///< @addr{0x809C4310}
};

} // namespace Field

} // namespace Kinoko
