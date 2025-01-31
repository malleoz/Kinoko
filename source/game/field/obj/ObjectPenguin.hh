#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectPenguin : public ObjectCollidable {
public:
    ObjectPenguin(const System::MapdataGeoObj &params);
    ~ObjectPenguin();

    void init() override;
    void calc() override;
    [[nodiscard]] u32 loadFlags() const override;

protected:
    void calcWalk();
    void calcPos();
    void calcRot();
    EGG::Vector3f FUN_80775BA4(f32 scale, const EGG::Vector3f &v0, const EGG::Vector3f &v1);

private:
    u32 m_state;
    EGG::Vector3f m_b0;
};

class ObjectPenguinM : public ObjectPenguin {
public:
    ObjectPenguinM(const System::MapdataGeoObj &params);
    ~ObjectPenguinM();
};

} // namespace Field
