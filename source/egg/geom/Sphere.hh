#pragma once

#include "egg/math/Vector.hh"

namespace Kinoko::EGG {

/// @brief Represents a sphere in 3D space
struct Sphere3f {
    /// @brief Default non-initializing constructor
    constexpr Sphere3f() = default;

    /// @brief Constructs a sphere from a position and a radius
    /// @param v The position of the sphere's center
    /// @param r The radius of the sphere
    constexpr Sphere3f(const Vector3f &v, f32 r) : pos(v), radius(r) {}

    /// @brief Default destructor
    constexpr ~Sphere3f() = default;

    /// @addr{0x8051A07C}
    /// @brief Checks if this sphere lies entirely inside another sphere
    /// @param rhs The other sphere to check against
    /// @return `true` if this sphere is completely inside rhs, `false` otherwise
    /// @details Not actually part of EGG. Included here for compartmentalization.
    [[nodiscard]] constexpr bool isInsideOtherSphere(const Sphere3f &rhs) const {
        f32 radiusDiff = rhs.radius - radius;
        if (radiusDiff < 0.0f) {
            return false;
        }

        return rhs.pos.ps_sqDistance(this->pos) < radiusDiff * radiusDiff;
    }

    Vector3f pos;
    f32 radius;
};

} // namespace Kinoko::EGG
