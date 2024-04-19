#pragma once

#include "egg/math/Vector.hh"

namespace EGG {

/// @brief Represents a sphere in 3D space.
/// @details This is not actually part of EGG, but is included here in order
/// to abstract away scenarios in which we need to compare spheres.
struct Sphere3f {
    Sphere3f(const Vector3f &v, f32 r);

    /// @return True if this sphere is completely inside rhs.
    bool isInsideOtherSphere(const Sphere3f &rhs) const;

    Vector3f pos;
    f32 radius;
};

} // namespace EGG
