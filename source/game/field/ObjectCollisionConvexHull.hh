#pragma once

#include "game/field/ObjectCollisionBase.hh"

namespace Kinoko::Field {

/// @brief Defines the collision for a convex hull
/// @details A convex hull is the smallest convex shape that encloses a given set of points.
class ObjectCollisionConvexHull : public ObjectCollisionBase {
public:
    ObjectCollisionConvexHull(const std::span<const EGG::Vector3f> &points);
    ~ObjectCollisionConvexHull() override;

    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) override;
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) override;
    const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const override;

    /// @beginGetters
    /// @addr{0x807F957C}
    [[nodiscard]] f32 getBoundingRadius() const override {
        return m_scaledRadius;
    }

    /// @addr{0x8081E254}
    [[nodiscard]] f32 initRadius() const {
        return m_initRadius;
    }
    /// @endGetters

    /// @beginSetters
    /// @addr{0x8080C414}
    void setBoundingRadius(f32 val) {
        m_scaledRadius = val;
    }
    /// @endSetters

protected:
    ObjectCollisionConvexHull(size_t count);

    owning_span<EGG::Vector3f> m_points; ///< Array of hull points in local space

private:
    const f32 m_initRadius; ///< Initial radius of the bounding sphere in world space
    owning_span<EGG::Vector3f> m_worldPoints; ///< Array of hull points transformed into world space
    f32 m_scaledRadius;                       ///< Scaled radius of the bounding sphere
};

} // namespace Kinoko::Field
