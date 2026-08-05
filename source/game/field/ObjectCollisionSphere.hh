#pragma once

#include "game/field/ObjectCollisionBase.hh"

namespace Kinoko::Field {

/// @brief Defines the collision for a spherical object
class ObjectCollisionSphere : public ObjectCollisionBase {
public:
    ObjectCollisionSphere(f32 radius, const EGG::Vector3f &center);
    ~ObjectCollisionSphere() override;

    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) override;
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) override;
    const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const override;

    /// @addr{0x80836B54}
    f32 getBoundingRadius() const override {
        return m_scaledRadius;
    }

private:
    bool m_hasTranslation;     ///< Whether the object has velocity or not
    const f32 m_radius;        ///< The radius of the sphere
    const EGG::Vector3f m_pos; ///< The position of the sphere in local space
    f32 m_scaledRadius;        ///< The scaled radius of the sphere
    EGG::Vector3f m_scaledPos; ///< The scaled position of the sphere in local space
    EGG::Vector3f m_worldPos;  ///< The position of the sphere in world space
    EGG::Vector3f m_center;    ///< The sphere's position in world space, offset by velocity
};

} // namespace Kinoko::Field
