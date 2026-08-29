#pragma once

#include "game/field/ObjectCollisionBase.hh"

namespace Kinoko::Field {

/// @brief Defines the collision for a spherical object
class ObjectCollisionSphere final : public ObjectCollisionBase {
public:
    /// @addr{0x808368D0}
    /// @brief Constructor
    /// @param radius Radius of the sphere
    /// @param center Center position of the sphere in local space
    ObjectCollisionSphere(f32 radius, const EGG::Vector3f &center)
        : m_hasTranslation(false), m_radius(radius), m_pos(center), m_scaledRadius(radius),
          m_scaledPos(center), m_worldPos(center) {}

    /// @addr{0x80836B5C}
    /// @brief Default virtual destructor
    ~ObjectCollisionSphere() override = default;

    /// @addr{0x80836998}
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) override {
        m_hasTranslation = false;

        if (scale.x != 1.0f) {
            m_scaledPos = m_pos * scale.x;
            m_scaledRadius = m_radius * scale.x;
        }

        m_worldPos = mat.multVector(m_scaledPos);
    }

    /// @addr{0x80836A50}
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) override {
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
    [[nodiscard]] const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const override {
        if (!m_hasTranslation) {
            return m_worldPos;
        }

        return m_worldPos.dot(v) > m_center.dot(v) ? m_worldPos : m_center;
    }

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
