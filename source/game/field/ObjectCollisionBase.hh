#pragma once

#include <egg/math/Matrix.hh>

namespace Kinoko::Field {

/// @brief Houses the internal state of a collision check in terms of the Gilbert–Johnson–Keerthi
/// (GJK) distance algorithm.
/// @details This collision check algorithm involves searching for the point on the Minkowski
/// difference of the two shapes closest to the origin. A Minkowski difference of two shapes
/// \f$A\f$ and \f$B\f$ is defined as:
/// \f[ A \ominus B = \{ a - b \mid a \in A,\ b \in B \} \f]
/// In other words, the Minkowski difference is every possible vector you get by subtracting a point
/// in \f$B\f$ from a point in \f$A\f$. Critically, shapes A and B intersect if and only if the
/// Minkowski difference contains the origin. For convex shapes, it can become expensive to compute
/// the Minkowski difference for a shape with a large number of vertices. Instead, for each shape
/// the GJK algorithm leverages a support function which can return the extreme point of each shape
/// in a given direction. Thus, we can find the Minkowski difference of \f$A\f$ and \f$B\f$ via:
/// \f[ \text{support}_{A \ominus B}(v) = \text{support}_A(v) - \text{support}_B(-v) \f]
/// Each support point is itself a vertex of \f$A \ominus B\f$, so the convex hull of a set of
/// support points sampled in different directions is a simplex inscribed within it:
/// \f[ \text{conv}(\{s_0, \dots, s_n\}) \subseteq A \ominus B, \quad
/// s_i = \text{support}_A(v_i) - \text{support}_B(-v_i) \f]
/// Rather than computing \f$A \ominus B\f$ directly, GJK iteratively grows this simplex by picking
/// new search directions until it encloses the origin (the shapes intersect) or converges on the
/// nearest point to the origin.\n
/// See more: https://en.wikipedia.org/wiki/Gilbert-Johnson-Keerthi_distance_algorithm
/// @todo Give a more detailed explanation on the significance of flags, mask, and m_candidateMask
struct GJKState {
    GJKState() : m_flags(0), m_idx(0), m_mask(0), m_candidateMask(0), m_scales{{}} {}

    u32 m_flags; ///< Bitmask indicating which slots are part of the active simplex
    u32 m_idx;   ///< Which slot (0-3) is being filled with a new support point this iteration
    u32 m_mask;  ///< `1 << m_idx`, the bit corresponding to that slot
    s32 m_candidateMask; ///< Candidate simplex bitmask (`m_flags | m_mask`) under evaluation
    std::array<EGG::Vector3f, 4> m_minDiffPts;   ///< Minkowski difference points
    std::array<EGG::Vector3f, 4> m_support1;     ///< Support point for the first object
    std::array<EGG::Vector3f, 4> m_support2;     ///< Support point for the second object
    std::array<std::array<f32, 4>, 16> m_scales; ///< Barycentric weights per simplex subset
};

/// @brief The abstract base class for collision representations of in-game objects
/// @details Implements collision detection between two convex objects via the GJK distance
/// algorithm. Exposes a pure virtual interface so that derived classes can implement their own
/// transformation, radius, and support point calculations.
///
/// \copydetails GJKState
/// @todo Document members
class ObjectCollisionBase {
    friend class Host::Context;

public:
    ObjectCollisionBase();
    virtual ~ObjectCollisionBase();

    /// @brief Transforms the collision object by a given matrix and scale
    /// @param mat The transformation matrix to apply to the collision object
    /// @param scale The scale to apply to the collision object
    virtual void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) = 0;

    /// @brief Transforms the collision object by a given matrix and scale, and caches the provided
    /// speed
    /// @param mat The transformation matrix to apply to the collision object
    /// @param scale The scale to apply to the collision object
    /// @param speed The object's movement velocity to cache
    virtual void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) = 0;

    /// @brief Returns the support point of the collision object in a given direction, i.e. the
    /// object's vertex with the greatest dot product with `v`
    /// @param v The direction to search in
    virtual const EGG::Vector3f &getSupport(const EGG::Vector3f &v) const = 0;

    /// @brief Returns the radius of the sphere bounding the collision object in world space
    virtual f32 getBoundingRadius() const = 0;

    [[nodiscard]] bool check(const ObjectCollisionBase &rhs, EGG::Vector3f &distance) const;

    /// @addr{0x80573520}
    [[nodiscard]] const EGG::Vector3f &velocity() const {
        return m_velocity;
    }

protected:
    EGG::Vector3f m_velocity; ///< Velocity of the associated object

private:
    [[nodiscard]] bool enclosesOrigin(const GJKState &state, u32 idx) const;
    void findNearestEnclosingSimplex(GJKState &state, EGG::Vector3f &v) const;
    [[nodiscard]] bool getNearestSimplex(GJKState &state, EGG::Vector3f &v) const;
    void getNearestPoint(GJKState &state, u32 idx, EGG::Vector3f &v0, EGG::Vector3f &v1) const;
    [[nodiscard]] bool isValidSimplex(const GJKState &state, u32 idx) const;
    [[nodiscard]] bool inSimplex(const GJKState &state, const EGG::Vector3f &v) const;
    void getNearestPoint(const GJKState &state, u32 idx, EGG::Vector3f &v) const;
    void calcSimplex(GJKState &state) const;

    /// @brief Scratch table for the dot products of the Minkowski difference points
    static std::array<std::array<f32, 4>, 4> s_dotProductCache;
};

} // namespace Kinoko::Field
