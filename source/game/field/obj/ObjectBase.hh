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
    virtual void createCollision() = 0;
    virtual void loadRail();
    virtual void calcCollisionTransform() = 0;

    /// @addr{0x80821DEC}
    virtual void disableCollision() const {
        m_boxColUnit->m_flag.setBit(eBoxColFlag::Intangible);
    }

    /// @addr{0x80821E00}
    virtual void enableCollision() const {
        m_boxColUnit->m_flag.resetBit(eBoxColFlag::Intangible);
    }

    /// @addr{0x806BF434}
    [[nodiscard]] virtual u32 loadFlags() const {
        // TODO: This references LOD to determine load flags
        return 0;
    }

    [[nodiscard]] virtual const char *getKclName() const;

    /// @addr{0x80681598}
    [[nodiscard]] virtual const EGG::Vector3f &getPosition() const {
        return m_pos;
    }

    /// @addr{0x8080BDC0}
    [[nodiscard]] virtual f32 getCollisionRadius() const {
        return 100.0f;
    }

    /// @addr{0x80572574}
    [[nodiscard]] virtual ObjectId id() const {
        return m_id;
    }

    void setFlag(u16 flag) {
        m_flags |= flag;
    }

    void setTransform(const EGG::Matrix34f &mat) {
        m_transform = mat;
    }

    void setPos(const EGG::Vector3f &pos) {
        m_pos = pos;
    }

protected:
    void calcTransform();
    void setMatrixTangentTo(const EGG::Vector3f &up, const EGG::Vector3f &tangent);

    ObjectId m_id;
    RailInterpolator *m_railInterpolator;
    BoxColUnit *m_boxColUnit;
    u16 m_flags;
    EGG::Vector3f m_pos;
    EGG::Vector3f m_rot;
    EGG::Vector3f m_scale;
    EGG::Matrix34f m_transform;
    const System::MapdataGeoObj *m_mapObj;
};

} // namespace Field
