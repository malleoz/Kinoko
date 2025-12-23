#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectCollisionCylinder;

class ObjectPropeller final : public ObjectCollidable {
public:
    ObjectPropeller(const System::MapdataGeoObj &params);
    ~ObjectPropeller() override;

    void init() override;
    void calc() override;

    /// @addr{0x80765BC0}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void createCollision() override;
    void calcCollisionTransform() override;
    [[nodiscard]] f32 getCollisionRadius() const override;
    bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) override;

private:
    f32 m_b4;
    f32 m_angle;             ///< offset 0xb8
    EGG::Vector3f m_axis;    ///< 0xbc
    EGG::Matrix34f m_rotMat; ///< 0xc8
    EGG::Matrix34f m_curRot; ///< 0xf8
    EGG::Matrix34f m_128;
    s32 m_168;
    std::array<ObjectCollisionCylinder *, 3> m_blades;
};

} // namespace Field
