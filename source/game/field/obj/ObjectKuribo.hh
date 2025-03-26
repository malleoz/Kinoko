#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectKuribo : public ObjectCollidable {
public:
    ObjectKuribo(const System::MapdataGeoObj &params);
    ~ObjectKuribo() override;

    void init() override;
    void calc() override;
    [[nodiscard]] u32 loadFlags() const override;
    void loadAnims() override;

private:
    void FUN_806DC220();
    void FUN_806DC3F8();
    void calcAnim();
    void calcRot();
    void checkSphereFull();
    EGG::Vector3f FUN_806dcd48(f32 scale, const EGG::Vector3f &v0, const EGG::Vector3f &v1) const;

    u16 m_b4;
    s32 m_b8;
    u32 m_cycleFrame;
    f32 m_speedStep;
    f32 m_animStep;
    EGG::Vector3f m_origin;
    f32 m_maxAnimTimer;
    u32 m_frameCount;
    f32 m_currSpeed;
    EGG::Vector3f m_rot;
    EGG::Vector3f m_floorNrm;
    f32 m_animTimer;
};

} // namespace Field
