#pragma once

#include "game/field/ObjectCollisionBase.hh"

namespace Kinoko::Field {

/// @brief Defines the collision for a convex hull
/// @details A convex hull is the smallest convex shape that encloses a given set of points.
class ObjectCollisionConvexHull : public ObjectCollisionBase {
public:
    /// @addr{0x808364E0}
    /// @brief Constructor that creates a convex hull with the provided points
    /// @param points Span of vertex positions defining the convex hull
    /// @details The base game has the possibility to only provide a count to allocate space.
    /// To account for this, we split the base game's constructor into two overloads.
    /// This overload enables conversion from std::array into a span, which initializes the points.
    /// Initializes @ref m_scaledRadius to @ref INIT_RADIUS.
    /// @warning The game stores the point array size as a single byte, so the number of points must
    /// be less than 256.
    ObjectCollisionConvexHull(const std::span<const EGG::Vector3f> &points)
        : m_points(points),
          m_worldPoints(points.size()),
          m_scaledRadius(INIT_RADIUS) {
        ASSERT(points.size() < 0x100);
    }

    /// @addr{0x808365A8}
    /// @brief Default virtual destructor
    ~ObjectCollisionConvexHull() override = default;

    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) override;
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) override;
    const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const override;

    /// @beginGetters

    /// @addr{0x807F957C}
    /// @copydetails ObjectCollisionBase::getBoundingRadius()
    [[nodiscard]] f32 getBoundingRadius() const override {
        return m_scaledRadius;
    }

    /// @endGetters

    /// @beginSetters

    /// @addr{0x8080C414}
    /// @brief Sets the scaled radius of the bounding sphere
    /// @param val The new scaled radius value
    void setBoundingRadius(f32 val) {
        m_scaledRadius = val;
    }

    /// @endSetters

    /// @addr{0x8081E254}
    /// @brief Gets the initial radius of the bounding sphere
    /// @return The initial radius of the bounding sphere, @ref INIT_RADIUS
    /// @details This is normally a non-static member function, but the initial radius is always set
    /// to @ref INIT_RADIUS in both constructors and never changes.
    [[nodiscard]] static f32 InitRadius() {
        return INIT_RADIUS;
    }

protected:
    /// @addr{0x808364E0}
    /// @brief Protected constructor that allocates space for a convex hull with `count` vertices
    /// @param count Number of points to allocate space for the convex hull
    /// @details The base game has the possibility to only provide a count to allocate space.
    /// To account for this, we split the base game's constructor into two overloads.
    /// This overload enables avoiding immediate point initialization, which is useful for
    /// inheritance. Initializes @ref m_scaledRadius to @ref INIT_RADIUS.
    /// @warning The game stores the point array size as a single byte, so the number of points must
    /// be less than 256.
    ObjectCollisionConvexHull(size_t count) : m_scaledRadius(INIT_RADIUS) {
        ASSERT(count < 0x100);
        m_points = owning_span<EGG::Vector3f>(count);
        m_worldPoints = owning_span<EGG::Vector3f>(count);
    }

    owning_span<EGG::Vector3f> m_points; ///< Array of hull vertices in local space

private:
    owning_span<EGG::Vector3f> m_worldPoints; ///< Array of hull points transformed into world space
    f32 m_scaledRadius;                       ///< Scaled radius of the bounding sphere

    /// @brief The initial radius of the bounding sphere for all convex hull objects
    static constexpr f32 INIT_RADIUS = 70.0f;
};

} // namespace Kinoko::Field
