#pragma once

#include "game/field/obj/ObjectBase.hh"

#include <egg/math/BoundBox.hh>

namespace Field {

class ObjectObakeBlock : public ObjectBase {
public:
    enum class FallState {
        Rest = 0,
        Falling = 1,
        Gone = 2,
    };

    ObjectObakeBlock(const System::MapdataGeoObj &params);
    ~ObjectObakeBlock() override;

    void calc() override;

    /// @addr{0x8080BDE0}
    void load() override {}

    /// @addr{0x8080BDDC}
    void createCollision() override {}

    /// @addr{0x8080BDD4}
    void calcCollisionTransform() override {}

    void setFallState(FallState state) {
        m_fallState = state;
    }

    [[nodiscard]] FallState fallState() {
        return m_fallState;
    }

    [[nodiscard]] u32 fallFrame() const {
        return m_fallFrame;
    }

private:
    EGG::Vector3f m_initialPos;
    FallState m_fallState;
    f32 m_framesFallen;
    EGG::Vector3f m_fallVel;
    EGG::Vector3f m_fallAngVel;
    EGG::BoundBox3f m_bbox;
    u32 m_fallFrame; ///< Frame the block starts falling, or 0 if it never falls.
};

} // namespace Field
