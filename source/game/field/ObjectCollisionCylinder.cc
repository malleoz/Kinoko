#include "ObjectCollisionCylinder.hh"

namespace Kinoko::Field {

/// @addr{0x80836068}
ObjectCollisionCylinder::ObjectCollisionCylinder(f32 radius, f32 height,
        const EGG::Vector3f &center)
    : m_radius(radius), m_height(height), m_pos(center) {
    m_scaledRadius = radius;
    m_scaledHeight = height;
    m_scaledPos = center;

    m_center = center;
    m_top = center + EGG::Vector3f::ey * height;
    m_bottom = center - EGG::Vector3f::ey * height;
}

/// @addr{0x808364A0}
ObjectCollisionCylinder::~ObjectCollisionCylinder() = default;

/// @addr{0x808361F0}
void ObjectCollisionCylinder::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) {
    m_scaledPos = m_pos * scale.x;
    m_scaledHeight = m_height * scale.y;
    m_scaledRadius = m_radius * scale.x;

    m_center = mat.ps_multVector(m_scaledPos);
    m_top = mat.ps_multVector(m_scaledPos + EGG::Vector3f::ey * m_scaledHeight);
    m_bottom = mat.ps_multVector(m_scaledPos - EGG::Vector3f::ey * m_scaledHeight);
}

/// @addr{0x80836334}
void ObjectCollisionCylinder::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
        const EGG::Vector3f &speed) {
    m_velocity = speed;
    ObjectCollisionCylinder::transform(mat, scale);
}

} // namespace Kinoko::Field
