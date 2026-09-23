#include "ObjectPropeller.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x807655B4}
/// @copybrief ObjectBase::createCollision()
/// @details Creates cylindrical collision for the shell and each of the 3 blades
void ObjectPropeller::createCollision() {
    ObjectCollidable::createCollision();

    const auto &colCenter = collisionCenter();
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto &params = flowTable.set(flowTable.slot(id()))->params.cylinder;
    f32 radius = static_cast<f32>(parse<s16>(params.radius));
    f32 height = static_cast<f32>(parse<s16>(params.height));

    for (auto *&blade : m_blades) {
        blade = EGG::egg_new<ObjectCollisionCylinder>(radius, height, colCenter);
    }
}

/// @addr{0x80765738}
/// @copybrief ObjectBase::calcCollisionTransform()
/// @details Rotates the transformation matrix of each blade around the propeller's center point,
/// such that the blades are spaced evenly at 120-degree intervals.
void ObjectPropeller::calcCollisionTransform() {
    constexpr f32 BLADE_LENGTH = 250.0f;

    for (u32 i = 0; i < m_blades.size(); ++i) {
        EGG::Matrix34f mat;
        mat.setBase(3, EGG::Vector3f::zero);
        mat.setAxisRotation(static_cast<f32>(i * 120) * DEG2RAD, m_axis);
        calcTransform();
        mat = mat.multiplyTo(transform());
        EGG::Vector3f dir = mat.base(1);
        dir.normalise();
        dir *= BLADE_LENGTH;
        mat.setBase(3, pos() + dir);

        auto *&blade = m_blades[i];
        blade->transform(mat, scale());
    }
}

/// @addr{0x80765930}
/// @copybrief ObjectBase::getCollisionRadius()
/// @return The collision radius of the propeller, calculated based on its bounding box.
f32 ObjectPropeller::getCollisionRadius() const {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto &params = flowTable.set(flowTable.slot(id()))->params.box;
    f32 z = scale().z * static_cast<f32>(parse<s16>(params.z));
    f32 x = scale().x * static_cast<f32>(parse<s16>(params.x));

    return 5.0f * std::max(z, x);
}

/// @addr{0x80765068}
/// @brief Calculates the propeller's rotation angle and updates its transformation matrix
/// @details @ref m_angle increments by half of @ref m_angVel. @ref m_curRot is computed by rotating
/// around @ref m_axis by @ref m_angle degrees. Finally, sets the transformation matrix to reflect
/// the updated rotation.
void ObjectPropeller::calcAngleAndRot() {
    m_angle += m_angVel * 0.5f;
    m_curRot = EGG::Matrix34f::ident;
    m_curRot.setAxisRotation(m_angle * DEG2RAD, m_axis);
    EGG::Matrix34f transform = m_curRot.multiplyTo(m_initMat);
    transform.setBase(3, pos());
    setTransform(transform);
}

} // namespace Kinoko::Field
