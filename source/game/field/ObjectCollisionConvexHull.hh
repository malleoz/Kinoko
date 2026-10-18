#pragma once

#include <Types.hh>

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
    [[nodiscard]] const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const override;
    [[nodiscard]] f32 getBoundingRadius() const override;

    /// @beginSetters

    void setBoundingRadius(f32 val);

    /// @endSetters

    [[nodiscard]] static f32 InitRadius();

protected:
    ObjectCollisionConvexHull(size_t count);

    owning_span<EGG::Vector3f> m_points; ///< Array of hull vertices in local space

private:
    owning_span<EGG::Vector3f> m_worldPoints; ///< Array of hull points transformed into world space
    f32 m_scaledRadius;                       ///< Scaled radius of the bounding sphere

    /// @brief The initial radius of the bounding sphere for all convex hull objects
    static constexpr f32 INIT_RADIUS = 70.0f;
};

} // namespace Kinoko::Field
