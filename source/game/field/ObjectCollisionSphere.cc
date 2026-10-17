#include "ObjectCollisionSphere.hh"

namespace Kinoko::Field {

/// @addr{0x808368D0}
/// @brief Constructor
/// @param radius Radius of the sphere
/// @param center Center position of the sphere in local space
/// @details Sets @ref m_radius and @ref m_scaledRadius to the provided `radius` and @ref m_pos,
/// @ref m_scaledPos, and @ref m_worldPos to the provided `center` position.
ObjectCollisionSphere::ObjectCollisionSphere(f32 radius, const EGG::Vector3f &center)
    : m_hasTranslation(false),
      m_radius(radius),
      m_pos(center),
      m_scaledRadius(radius),
      m_scaledPos(center),
      m_worldPos(center) {}

/// @addr{0x80836B5C}
/// @brief Default virtual destructor
ObjectCollisionSphere::~ObjectCollisionSphere() = default;

/// @addr{0x80836998}
/// @copydoc ObjectCollisionBase::transform(const EGG::Matrix34f &, const EGG::Vector3f &)
/// @details Transforms the sphere's position and radius according to the provided `mat` and
/// `scale`.
void ObjectCollisionSphere::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) {
    m_hasTranslation = false;

    if (scale.x != 1.0f) {
        m_scaledPos = m_pos * scale.x;
        m_scaledRadius = m_radius * scale.x;
    }

    m_worldPos = mat.multVector(m_scaledPos);
}

/// @addr{0x80836A50}
/// @copydoc ObjectCollisionBase::transform(const EGG::Matrix34f &, const EGG::Vector3f &, const
/// EGG::Vector3f &)
/// @details Transforms the sphere's position and radius according to the provided `mat` and
/// `scale`. Sets @ref m_velocity based on the provided `speed`. Finally, updates the sphere's
/// @ref m_center to reflect its new position in world space offset by the velocity.
void ObjectCollisionSphere::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
        const EGG::Vector3f &speed) {
    m_hasTranslation = true;
    m_velocity = speed;

    if (scale.x != 1.0f) {
        m_scaledPos = m_pos * scale.x;
        m_scaledRadius = m_radius * scale.x;
    }

    m_worldPos = mat.multVector(m_scaledPos);

    m_center = m_worldPos - speed;
}

/// @addr{0x80836920}
/// @copydoc ObjectCollisionBase::getSupport()
/// @details If the object has no translation (velocity), it returns @ref m_worldPos. Otherwise,
/// it returns the point between @ref m_worldPos and @ref m_center that is furthest along the
/// direction of `v`.
const EGG::Vector3f &ObjectCollisionSphere::getSupport(const EGG::Vector3f &v) const {
    if (!m_hasTranslation) {
        return m_worldPos;
    }

    return m_worldPos.dot(v) > m_center.dot(v) ? m_worldPos : m_center;
}

/// @addr{0x80836B54}
/// @copydoc ObjectCollisionBase::getBoundingRadius()
f32 ObjectCollisionSphere::getBoundingRadius() const {
    return m_scaledRadius;
}

} // namespace Kinoko::Field
