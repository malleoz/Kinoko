#include "KartBody.hh"

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartPhysics.hh"

namespace Kinoko::Kart {

/// @addr{0x8056C394}
/// @brief Constructor
/// @param physics Pointer to the kart's physics state manager
/// @details Saves a pointer to the kart's physics state manager to @ref m_physics. Initializes
/// @ref m_leanAngle, @ref m_sinkDepth, and @ref m_targetSinkDepth to zero.
KartBody::KartBody(KartPhysics *physics) : m_physics(physics) {
    m_leanAngle = 0.0f;
    m_sinkDepth = 0.0f;
    m_targetSinkDepth = 0.0f;
}

/// @addr{0x8056CD84}
/// @brief Default virtual destructor
/// @note The derived classes are responsible for properly deleting the @ref KartPhysics object
KartBody::~KartBody() = default;

/// @addr{0x8056C604}
/// @brief Computes a matrix to represent wheel rotation. For karts, this is wheel-agnostic.
/// @param bodyMat The kart's full rotation and position as a matrix
/// @return The transformation matrix representing the wheel's rotation
/// @details For karts, this is simply the kart's full rotation and position.
/// @note Kinoko-specific: the caller computes `bodyMat` once for all wheels, rather than once per
/// wheel.
EGG::Matrix34f KartBody::wheelMatrix(u16, const EGG::Matrix34f &bodyMat) {
    return bodyMat;
}

/// @addr{0x8056C4B4}
/// @brief Resets the kart's body to default values
/// @details Calls @ref KartPhysics::reset() on @ref m_physics. Resets @ref m_leanAngle, @ref
/// m_sinkDepth, and @ref m_targetSinkDepth to zero.
void KartBody::reset() {
    m_physics->reset();
    m_leanAngle = 0.0f;
    m_sinkDepth = 0.0f;
    m_targetSinkDepth = 0.0f;
}

/// @addr{0x8056C9C4}
/// @brief Interpolates the current sink depth towards the target sink depth
/// @details Smoothly updates @ref m_sinkDepth to approach @ref m_targetSinkDepth using a fixed
/// `0.1f` interpolation rate.
void KartBody::calcSinkDepth() {
    constexpr f32 SINK_INTERP_RATE = 0.1f;

    m_sinkDepth += (m_targetSinkDepth - m_sinkDepth) * SINK_INTERP_RATE;
}

/// @addr{0x8056C950}
/// @brief Sets the target sink depth to the provided value if it's greater than the current
/// target sink depth
/// @param val The new target sink depth to try to set
void KartBody::trySetTargetSinkDepth(f32 val) {
    m_targetSinkDepth = std::max(val, m_targetSinkDepth);
}

/// @addr{0x8056C964}
/// @brief Updates the target sink depth based off of the current collision's intensity
/// @details Sets @ref m_targetSinkDepth to three times the intensity of the current collision.
void KartBody::calcTargetSinkDepth() {
    m_targetSinkDepth = 3.0f * static_cast<f32>(collisionData().intensity);
}

/// @addr{0x8056E424}
/// @brief Sets the lean angle of the kart
/// @param val The new lean angle to set
void KartBody::setLeanAngle(f32 val) {
    m_leanAngle = val;
}

/// @brief Gets a pointer to the underlying @ref KartPhysics instance
/// @return A pointer to the underlying @ref KartPhysics instance, @ref m_physics
KartPhysics *KartBody::physics() const {
    return m_physics;
}

/// @brief Gets the current sink depth of the kart
/// @return The current sink depth, @ref m_sinkDepth
f32 KartBody::sinkDepth() const {
    return m_sinkDepth;
}

/// @addr{0x8056CCC0}
/// @brief Constructor
KartBodyKart::KartBodyKart(KartPhysics *physics) : KartBody(physics) {}

/// @addr{0x8056E494}
/// @brief Virtual destructor that destroys the underlying @ref KartPhysics subsystem
KartBodyKart::~KartBodyKart() {
    EGG::egg_delete(m_physics);
}

/// @addr{0x8056D858}
/// @brief Constructor
/// @param physics Pointer to the kart's physics state manager
KartBodyBike::KartBodyBike(KartPhysics *physics) : KartBody(physics) {}

/// @addr{0x8056E2BC}
/// @brief Virtual destructor that destroys the underlying @ref KartPhysics subsystem
KartBodyBike::~KartBodyBike() {
    EGG::egg_delete(m_physics);
}

/// @addr{0x8056DD54}
/// @brief Computes a matrix to represent the rotation of a wheel.
/// @param wheelIdx 0 for front wheel, 1 for rear wheel
/// @param bodyMat The bike's full rotation and position as a matrix
/// @return The transformation matrix representing the wheel's rotation
/// @details For the front wheel, we factor in the handlebar rotation.
/// For the rear wheel, we only factor in the kart's rotation.
EGG::Matrix34f KartBodyBike::wheelMatrix(u16 wheelIdx, const EGG::Matrix34f &bodyMat) {
    if (wheelIdx != 0) {
        return bodyMat;
    }

    if (!m_handleRotMat.has_value()) {
        m_handleRotMat.emplace().makeR(DEG2RAD * param()->bikeDisp().m_handleRot);
    }

    EGG::Matrix34f handleMatrix = *m_handleRotMat;
    handleMatrix.setBase(3, param()->bikeDisp().m_handlePos * scale());
    EGG::Matrix34f tmp = bodyMat.multiplyTo(handleMatrix);

    EGG::Vector3f yRotation = EGG::Vector3f(0.0f, DEG2RAD * m_leanAngle, 0.0f);
    EGG::Matrix34f yRotMatrix;
    yRotMatrix.makeR(yRotation);
    return tmp.multiplyTo(yRotMatrix);
}

/// @addr{0x8056E218}
/// @brief Constructor
/// @param physics Pointer to the kart's physics state manager
KartBodyQuacker::KartBodyQuacker(KartPhysics *physics) : KartBodyBike(physics) {}

/// @addr{0x8056E42C}
/// @brief Default virtual destructor
KartBodyQuacker::~KartBodyQuacker() = default;

/// @addr{0x8056E2FC}
/// @brief Computes a matrix to represent wheel rotation. For Quacker, this is wheel-agnostic.
/// @param bodyMat The bike's full rotation and position as a matrix
/// @return The transformation matrix representing the wheel's rotation
EGG::Matrix34f KartBodyQuacker::wheelMatrix(u16 /* wheelIdx */, const EGG::Matrix34f &bodyMat) {
    return bodyMat;
}

} // namespace Kinoko::Kart
