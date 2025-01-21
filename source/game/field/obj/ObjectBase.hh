#pragma once

#include "game/field/BoxColManager.hh"
#include "game/field/RailInterpolator.hh"
#include "game/field/obj/ObjectId.hh"

#include "game/system/map/MapdataGeoObj.hh"

#include <egg/math/Matrix.hh>

namespace Field {

class ObjectBase {
public:
    ObjectBase(const System::MapdataGeoObj &params);
    virtual ~ObjectBase();

    virtual void init() {}
    virtual void calc() {}
    virtual void calcModel();
    virtual void load() = 0;
    virtual void loadRail();
    virtual void calcCollisionTransform() = 0;
    [[nodiscard]] virtual u32 loadFlags() const;
    [[nodiscard]] virtual const EGG::Vector3f &getPosition() const;
    [[nodiscard]] virtual f32 getCollisionRadius() const;

    [[nodiscard]] ObjectId id() const;

protected:
    void calcTransform();
    void setMatrixTangentTo(const EGG::Vector3f &up, const EGG::Vector3f &tangent);
    void FUN_808218B0(const EGG::Vector3f &v);
    [[nodiscard]] EGG::Matrix34f FUN_806B3CA4(const EGG::Vector3f &v);

    ObjectId m_id;
    RailInterpolator *m_railInterpolator;
    BoxColUnit *m_boxColUnit;
    u16 m_flags;
    EGG::Vector3f m_pos;
    EGG::Vector3f m_rot;
    EGG::Vector3f m_scale;
    EGG::Matrix34f m_transform;
    const System::MapdataGeoObj &m_globalObj;
};

} // namespace Field
