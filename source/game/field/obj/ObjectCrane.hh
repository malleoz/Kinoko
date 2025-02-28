#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Field {

class ObjectCrane final : public ObjectKCL {
public:
    ObjectCrane(const System::MapdataGeoObj &params);
    ~ObjectCrane() override;

    void calc() override;
    [[nodiscard]] u32 loadFlags() const override;

    [[nodiscard]] f32 colRadiusAdditionalLength() const override;

private:
    EGG::Vector3f m_startPos;
    EGG::Vector3f m_startRot;
    EGG::Vector3f m_startScale;
    u16 m_t;
    u16 _12a;
    u16 m_period1Denom;
    u16 m_period2Denom;
    u16 _130;
    u16 _132;
    f32 m_period1;
    f32 m_period2;
};

} // namespace Field
