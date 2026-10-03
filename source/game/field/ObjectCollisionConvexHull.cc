#include "ObjectCollisionConvexHull.hh"

namespace Kinoko::Field {

/// @addr{0x808366D0}
/// @copydoc ObjectCollisionBase::transform(const EGG::Matrix34f &, const EGG::Vector3f &)
/// @details Transforms the convex hull's vertices into world space using the provided
/// transformation matrix and scale vector. If the scale is not `1.0f`, a temporary scaling matrix
/// is applied before the transformation.
void ObjectCollisionConvexHull::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) {
    if (scale.x != 1.0f) {
        EGG::Matrix34f temp;
        temp.makeS(EGG::Vector3f(scale.x, scale.x, scale.x));
        temp = mat.multiplyTo(temp);

        for (size_t i = 0; i < m_points.size(); ++i) {
            m_worldPoints[i] = temp.ps_multVector(m_points[i]);
        }
    } else {
        for (size_t i = 0; i < m_points.size(); ++i) {
            m_worldPoints[i] = mat.ps_multVector(m_points[i]);
        }
    }
}

/// @addr{0x808367C4}
/// @copydoc ObjectCollisionBase::transform(const EGG::Matrix34f &, const EGG::Vector3f &, const
/// EGG::Vector3f &)
/// @details Sets @ref m_velocity to the provided `speed`. Transforms the convex hull's vertices
/// into world space using the provided transformation matrix and scale vector. If the scale is not
/// `0.0f`, a temporary scaling matrix is applied before the transformation.
void ObjectCollisionConvexHull::transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
        const EGG::Vector3f &speed) {
    m_velocity = speed;

    if (scale.x == 0.0f) {
        for (size_t i = 0; i < m_points.size(); ++i) {
            m_worldPoints[i] = mat.ps_multVector(m_points[i]);
        }
    } else {
        EGG::Matrix34f temp;
        temp.makeS(EGG::Vector3f(scale.x, scale.x, scale.x));
        temp = mat.multiplyTo(temp);

        for (size_t i = 0; i < m_points.size(); ++i) {
            m_worldPoints[i] = temp.ps_multVector(m_points[i]);
        }
    }
}

/// @addr{0x80836628}
/// @copydoc ObjectCollisionBase::getSupport()
/// @details Finds the vertex of the convex hull that has the greatest dot product with the given
/// direction `v`.
const EGG::Vector3f &ObjectCollisionConvexHull::getSupport(const EGG::Vector3f &v) const {
    const EGG::Vector3f *result = &m_worldPoints[0];
    f32 maxDot = v.dot(*result);

    for (size_t i = 1; i < m_worldPoints.size(); ++i) {
        const auto &iter = m_worldPoints[i];
        f32 iterDot = v.dot(iter);

        if (maxDot < iterDot) {
            result = &iter;
            maxDot = iterDot;
        }
    }

    return *result;
}

} // namespace Kinoko::Field
