#pragma once

#include "game/field/KCollisionTypes.hh"
#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectDrivable.hh"

#include <egg/core/Disposer.hh>

#include <vector>

namespace Field {

class ObjectDrivableDirector : EGG::Disposer {
public:
    void init();
    void calc();
    void addObject(ObjectDrivable *obj);

    bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut,
            u32 timeOffset);
    bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut,
            u32 timeOffset);
    bool checkSphereFull(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfo *info, KCLTypeMask *maskOut,
            u32 timeOffset);
    bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfo *info, KCLTypeMask *maskOut,
            u32 timeOffset);
    bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset);
    bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset);
    bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    void colNarScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask, u32 timeOffset);

    static ObjectDrivableDirector *CreateInstance();
    static void DestroyInstance();
    static ObjectDrivableDirector *Instance();

private:
    ObjectDrivableDirector();
    ~ObjectDrivableDirector() override;

    // TODO: Migrate to vabold's fixed_vector
    std::vector<ObjectDrivable *> m_objects; ///< All objects live here
    std::vector<ObjectBase *> m_calcObjects; ///< Objects needing calc() live here too.

    static ObjectDrivableDirector *s_instance;
};

} // namespace Field
