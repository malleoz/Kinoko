#include "ObjectCollisionCylinder.hh"

namespace Kinoko::Field {

/// @addr{0x80836068}
/// @brief Constructor
/// @param radius Radius of the cylinder
/// @param height Height of the cylinder
/// @param center Center position of the cylinder in local space
/// @details Initializes @ref m_scaledRadius, @ref m_scaledHeight, and @ref m_scalesPos based on
/// their world counterparts. Sets @ref m_center based on the provided `center`, and computes
/// @ref m_top and @ref m_bottom based on the `center` and the `height`.
ObjectCollisionCylinder::ObjectCollisionCylinder(f32 radius, f32 height,
        const EGG::Vector3f &center)
    : m_radius(radius),
      m_height(height),
      m_pos(center) {
    m_scaledRadius = radius;
    m_scaledHeight = height;
    m_scaledPos = center;

    m_center = center;
    m_top = center + EGG::Vector3f::ey * height;
    m_bottom = center - EGG::Vector3f::ey * height;
}

/// @addr{0x808364A0}
/// @brief Default virtual destructor
ObjectCollisionCylinder::~ObjectCollisionCylinder() = default;

/// @addr{0x808361F0}
/// @copydoc ObjectCollisionBase::transform(const EGG::Matrix34f &, const EGG::Vector3f &)
/// @details Updates the cylinder's center, top, and bottom points in world space using the
/// provided transformation matrix and scale vector. Updates @ref m_scaledPos, @ref
/// m_scaledHeight, and @ref m_scaledRadius based on the provided `scale`.
void ObjectCollisionCylinder::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) {
    m_scaledPos = m_pos * scale.x;
    m_scaledHeight = m_height * scale.y;
    m_scaledRadius = m_radius * scale.x;

    m_center = mat.ps_multVector(m_scaledPos);
    m_top = mat.ps_multVector(m_scaledPos + EGG::Vector3f::ey * m_scaledHeight);
    m_bottom = mat.ps_multVector(m_scaledPos - EGG::Vector3f::ey * m_scaledHeight);
}

/// @addr{0x80836334}
/// @copydoc ObjectCollisionBase::transform(const EGG::Matrix34f &, const EGG::Vector3f &, const
/// EGG::Vector3f &)
/// @details Sets @ref m_velocity to the provided `speed` and then updates the cylinder's scaled
/// radius, height, and position based on the provided transformation matrix and scale vector.
void ObjectCollisionCylinder::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
        const EGG::Vector3f &speed) {
    m_velocity = speed;
    transform(mat, scale);
}

/// @addr{0x8083618C}
/// @copydoc ObjectCollisionBase::getSupport()
/// @details Returns either the top or the bottom point of the cylinder, depending on which has
/// the greatest dot product with `v`.
const EGG::Vector3f &ObjectCollisionCylinder::getSupport(const EGG::Vector3f &v) const {
    return m_top.dot(v) > m_bottom.dot(v) ? m_top : m_bottom;
}

/// @addr{0x80836498}
/// @copydoc ObjectCollisionBase::getBoundingRadius()
f32 ObjectCollisionCylinder::getBoundingRadius() const {
    return m_scaledRadius;
}

} // namespace Kinoko::Field
