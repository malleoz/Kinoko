#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectFireball : public ObjectCollidable {
public:
    /// @addr{0x80768650}
    ObjectFireball(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x807689AC}
    ~ObjectFireball() = default;

    /// @addr{0x80768728}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8076871C}
    [[nodiscard]] const char *getKclName() const override {
        return "fireBPlane";
    }

    void setScalar(f32 scalar) {
        m_scalar = scalar;
    }

    void setb8(f32 val) {
        m_b8 = val;
    }

    [[nodiscard]] f32 scalar() const {
        return m_scalar;
    }

    [[nodiscard]] f32 b8() const {
        return m_b8;
    }

private:
    f32 m_scalar; // 0xb4
    f32 m_b8;
};

} // namespace Field
