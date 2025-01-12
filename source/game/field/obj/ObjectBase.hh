#pragma once

#include "game/field/BoxColManager.hh"
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
    virtual void calcCollisionTransform() = 0;
    [[nodiscard]] virtual ObjectId id() const;
    [[nodiscard]] virtual u32 loadFlags() const;
    [[nodiscard]] virtual const char *getModelName() const;
    [[nodiscard]] virtual const EGG::Vector3f &getPosition() const;
    [[nodiscard]] virtual f32 getCollisionRadius() const;
    virtual void createCollision() = 0;

    /// @beginGetters
    [[nodiscard]] const EGG::Matrix34f &transform() const;
    /// @endGetters

protected:
    void calcTransform();

    ObjectId m_id;
    BoxColUnit *m_boxColUnit;
    u16 m_flags;
    EGG::Vector3f m_pos;
    EGG::Vector3f m_rot;
    EGG::Vector3f m_scale;
    EGG::Matrix34f m_transform;
};

} // namespace Field
