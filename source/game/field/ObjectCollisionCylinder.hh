#pragma once

#include "game/field/ObjectCollisionBase.hh"

namespace Kinoko::Field {

/// @brief Defines the collision for a cylindrical object
/// @details The cylinder is defined by a radius and height, and is centered at a given position.
/// The class caches the top and bottom points of the cylinder for efficient support point
/// calculations.
class ObjectCollisionCylinder final : public ObjectCollisionBase {
public:
    /// @addr{0x80836068}
    /// @brief Constructor
    /// @param radius Radius of the cylinder
    /// @param height Height of the cylinder
    /// @param center Center position of the cylinder in local space
    ObjectCollisionCylinder(f32 radius, f32 height, const EGG::Vector3f &center)
        : m_radius(radius), m_height(height), m_pos(center) {
        m_scaledRadius = radius;
        m_scaledHeight = height;
        m_scaledPos = center;

        m_center = center;
        m_top = center + EGG::Vector3f::ey * height;
        m_bottom = center - EGG::Vector3f::ey * height;
    }

    /// @addr{0x808364A0}
    /// @brief Default virtual destructor
    ~ObjectCollisionCylinder() override = default;

    /// @addr{0x808361F0}
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) override {
        m_scaledPos = m_pos * scale.x;
        m_scaledHeight = m_height * scale.y;
        m_scaledRadius = m_radius * scale.x;

        m_center = mat.ps_multVector(m_scaledPos);
        m_top = mat.ps_multVector(m_scaledPos + EGG::Vector3f::ey * m_scaledHeight);
        m_bottom = mat.ps_multVector(m_scaledPos - EGG::Vector3f::ey * m_scaledHeight);
    }

    /// @addr{0x80836334}
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) override {
        m_velocity = speed;
        transform(mat, scale);
    }

    /// @addr{0x8083618C}
    /// @details Returns either the top or the bottom point of the cylinder, depending on which has
    /// the greatest dot product with `v`.
    const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const override {
        return m_top.dot(v) > m_bottom.dot(v) ? m_top : m_bottom;
    }

    /// @addr{0x80836498}
    f32 getBoundingRadius() const override {
        return m_scaledRadius;
    }

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
