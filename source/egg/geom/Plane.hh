#pragma once

#include "egg/math/Vector.hh"

namespace Kinoko::EGG {

/// @brief Represents a plane in 3D space, expressed with `n.dot(x) = d` for point `x` on the plane.
/// @details Documentation from Real-Time Collision Detection by Christer Ericson,
/// published by Morgan Kaufmann Publishers, Copyright 2005 Elsevier Inc.
struct Plane3f {
    /// @addr{0x805AF020}
    /// @brief Default non-initializing constructor
    constexpr Plane3f() = default;

    /// @addr{0x805AEF6C}
    /// @brief Constructs a plane from a point and a normal vector
    /// @param point A point on the plane
    /// @param normal The normal vector of the plane
    constexpr Plane3f(const Vector3f &point, const Vector3f &normal) {
        set(point, normal);
    }

    /// @addr{0x805AF330}
    /// @brief Default destructor
    constexpr ~Plane3f() = default;

    /// @brief Copy constructor
    constexpr Plane3f(const Plane3f &) = default;

    /// @brief Copy assignment operator
    constexpr Plane3f &operator=(const Plane3f &) = default;

    /// @addr{0x805AF048}
    /// @brief Sets the plane based on a point and a normal vector
    /// @param point A point on the plane
    /// @param normal The normal vector of the plane
    constexpr void set(const Vector3f &point, const Vector3f &normal) {
        n = normal;
        d = -(normal.dot(point));
    }

    /// @addr{0x805AF0F0}
    /// @brief Tests whether a point is on the plane
    /// @param point The point to test against the plane
    /// @return `true` if the point is on the plane, `false` otherwise
    [[nodiscard]] constexpr bool testPoint(const Vector3f &point) const {
        return d + n.dot(point) <= 0.0f;
    }

    Vector3f n; ///< Plane normal
    f32 d;      ///< Negated dot product. `n.dot(x) = d => n.dot(x) - d = 0`
};

} // namespace Kinoko::EGG
