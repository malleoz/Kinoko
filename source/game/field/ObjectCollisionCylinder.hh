#pragma once

#include "game/field/ObjectCollisionBase.hh"

namespace Kinoko::Field {

/// @brief Defines the collision for a cylindrical object
/// @details The cylinder is defined by a radius and height, and is centered at a given position.
/// The class caches the top and bottom points of the cylinder for efficient support point
/// calculations.
class ObjectCollisionCylinder final : public ObjectCollisionBase {
public:
    ObjectCollisionCylinder(f32 radius, f32 height, const EGG::Vector3f &center);
    ~ObjectCollisionCylinder() override;

    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) override;
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) override;
    [[nodiscard]] const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const override;
    [[nodiscard]] f32 getBoundingRadius() const override;

private:
    const f32 m_radius;        ///< The radius of the cylinder
    const f32 m_height;        ///< The height of the cylinder
    const EGG::Vector3f m_pos; ///< The center position of the cylinder in local space

    f32 m_scaledRadius;        ///< The scaled radius of the cylinder
    f32 m_scaledHeight;        ///< The scaled height of the cylinder
    EGG::Vector3f m_scaledPos; ///< The scaled center position of the cylinder in world space

    EGG::Vector3f m_center; ///< The center position of the cylinder in world space
    EGG::Vector3f m_top;    ///< The top point of the cylinder in world space
    EGG::Vector3f m_bottom; ///< The bottom point of the cylinder in world space
};

} // namespace Kinoko::Field
