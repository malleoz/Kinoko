#pragma once

#include "game/field/obj/ObjectKCL.hh"

#include <optional>

namespace Field {

class ObjectSandcone : public ObjectKCL {
public:
    ObjectSandcone(const System::MapdataGeoObj &params);
    ~ObjectSandcone() override;

    void init() override;
    void calc() override;
    [[nodiscard]] u32 loadFlags() const override;

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

private:
    f32 m_flowRate;
    f32 m_finalHeightDelta;
    u16 m_startFrame;
    u16 m_duration;
    EGG::Matrix34f m_rtMtx;
    EGG::Matrix34f m_currentMtx;

    // Performance improvement to avoid recomputing the highest position a sandcone will attain.
    std::optional<EGG::Vector3f> m_cachedFinalPos;
};

} // namespace Field
