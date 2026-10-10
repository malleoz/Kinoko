#include "KartDynamics.hh"

namespace Kinoko::Kart {

/// @addr{0x805B4B54}
/// @brief Initializes the kart's dynamics to default values
/// @details Initializess all position and velocity vectors to zero. Sets @ref m_speedNorm to zero.
/// Initializes all rotation quaternions to the identity quaternion. Sets gravity to `-1.0f`.
/// Initializes @ref m_up and @ref m_stabilizeUp to the world up vector. Sets @ref m_forceUpright to
/// `true`. Sets @ref m_noGravity and @ref m_killExtVelY to `false` and @ref m_stabilizationFactor
/// to `0.1f`. Initializes @ref m_headingExtVel and @ref m_angVel0YFactor to `0.0f`. Sets @ref
/// m_scale to the unit vector.
void KartDynamics::init() {
    m_pos = EGG::Vector3f::zero;
    m_extVel = EGG::Vector3f::zero;
    m_acceleration = EGG::Vector3f::zero;
    m_angVel0 = EGG::Vector3f::zero;
    m_movingObjVel = EGG::Vector3f::zero;
    m_angVel1 = EGG::Vector3f::zero;
    m_movingRoadVel = EGG::Vector3f::zero;
    m_velocity = EGG::Vector3f::zero;
    m_speedNorm = 0.0f;
    m_angVel2 = EGG::Vector3f::zero;
    m_mainRot = EGG::Quatf::ident;
    m_fullRot = EGG::Quatf::ident;
    m_totalForce = EGG::Vector3f::zero;
    m_totalTorque = EGG::Vector3f::zero;
    m_stuntRot = EGG::Quatf::ident;
    m_extraRot = EGG::Quatf::ident;
    m_gravity = -1.0f;
    m_intVel = EGG::Vector3f::zero;
    m_up = EGG::Vector3f::ey;
    m_forceUpright = true;
    m_noGravity = false;
    m_killExtVelY = false;
    m_stabilizationFactor = 0.1f;
    m_stabilizeUp = EGG::Vector3f::ey;
    m_headingExtVel = 0.0f;
    m_angVel0YFactor = 0.0f;
    m_scale = EGG::Vector3f::unit;
}

/// @addr{0x805B5170}
/// @brief Every frame, computes acceleration, velocity, position, and rotation of the kart.
/// @param dt Delta time, always 1.0f
/// @param maxSpeed Maximum speed of the kart, always 120.0f
/// @param air `true` if the kart is in the air, `false` otherwise.
/// @details Applies acceleration from gravity and total force to external velocity. Decays external
/// velocity by 0.2% and angular velocity by 2% each frame. Extracts the forward/backward component
/// from external velocity and stores it in @ref m_headingExtVel. If mid-air, caps the internal
/// velocity at `120.0f`. Computes overall velocity from external, internal, and moving object/road
/// velocities. Updates position accordingly. Calculates overall angular velocity, stabilizes the
/// vehicle, and applies the resulting rotation.
void KartDynamics::calc(f32 dt, f32 maxSpeed, bool air) {
    constexpr f32 EXT_VEL_DECAY = 0.998f;
    constexpr f32 ANG_VEL_DECAY = 0.98f;
    constexpr f32 TERMINAL_Y_VEL = 120.0f;

    if (!m_noGravity) {
        m_totalForce.y += m_gravity;
    }

    m_acceleration = m_totalForce;
    m_extVel += m_acceleration * dt;

    if (m_killExtVelY) {
        m_extVel.y = std::min(0.0f, m_extVel.y);
    }

    m_extVel *= EXT_VEL_DECAY;
    m_angVel0 *= ANG_VEL_DECAY;

    EGG::Vector3f forward = m_mainRot.rotateVector(EGG::Vector3f::ez);
    EGG::Vector3f forwardXZ = forward;
    forwardXZ.y = 0.0f;

    if (forwardXZ.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        forwardXZ.normalise();
        const auto [proj, rej] = m_extVel.projAndRej(forwardXZ);
        const EGG::Vector3f &forwardExtVel = proj;
        m_extVel = rej;

        f32 forwardSpeed = forwardExtVel.squaredLength();
        forwardSpeed = forwardSpeed > std::numeric_limits<f32>::epsilon() ?
                EGG::Mathf::sqrt(forwardSpeed) :
                0.0f;

        m_headingExtVel = forwardSpeed * forward.dot(forwardXZ);
        if (forwardExtVel.dot(forwardXZ) < 0.0f) {
            m_headingExtVel = -m_headingExtVel;
        }
    }

    if (air) {
        m_intVel.y = std::min(TERMINAL_Y_VEL, m_intVel.y);
    }

    m_velocity = m_extVel * dt + m_intVel + m_movingObjVel + m_movingRoadVel;

    if (m_scale.z > 1.0f) {
        maxSpeed *= m_scale.z;
    }

    m_speedNorm = std::min(m_velocity.normalise(), maxSpeed);
    m_velocity *= m_speedNorm;
    m_pos += m_velocity;

    EGG::Vector3f t1 = m_invInertiaTensor.multVector33(m_totalTorque) * dt;
    m_angVel0 += (t1 + m_invInertiaTensor.multVector33(t1 + m_totalTorque) * dt) * 0.5f;

    m_angVel0.x = std::min(0.4f, std::max(-0.4f, m_angVel0.x));
    m_angVel0.y = std::min(0.4f, std::max(-0.4f, m_angVel0.y)) * m_angVel0YFactor;
    m_angVel0.z = std::min(0.8f, std::max(-0.8f, m_angVel0.z));

    if (m_forceUpright) {
        forceUpright();
    }

    EGG::Vector3f angVelSum = m_angVel2 + m_angVel1 + m_angVel0Factor * m_angVel0;

    if (angVelSum.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_mainRot += m_mainRot.multSwap(angVelSum) * (dt * 0.5f);

        if (EGG::Mathf::abs(m_mainRot.squaredNorm()) > std::numeric_limits<f32>::epsilon()) {
            m_mainRot.normalise();
        } else {
            m_mainRot = EGG::Quatf::ident;
        }
    }

    if (m_forceUpright) {
        stabilize();
    }

    if (EGG::Mathf::abs(m_mainRot.squaredNorm()) > std::numeric_limits<f32>::epsilon()) {
        m_mainRot.normalise();
    } else {
        m_mainRot = EGG::Quatf::ident;
    }

    m_fullRot = m_extraRot.multSwap(m_mainRot).multSwap(m_stuntRot);
    m_fullRot.normalise();

    m_totalForce.setZero();
    m_totalTorque.setZero();
    m_angVel2.setZero();
}

/// @addr{0x805B4D24}
/// @brief Resets velocity, acceleration, angular velocity, and total force/torque to zero.
void KartDynamics::reset() {
    m_extVel.setZero();
    m_acceleration.setZero();
    m_angVel0.setZero();
    m_movingObjVel.setZero();
    m_angVel1.setZero();
    m_movingRoadVel.setZero();
    m_angVel2.setZero();
    m_totalForce.setZero();
    m_totalTorque.setZero();
    m_intVel.setZero();
}

/// @addr{0x805B6150}
/// @brief Every frame, computes torque from rotation and vertical linear motion.
/// @param pos Position of the rigid body.
/// @param fLinear Linear motion force
/// @param fRot Rotational force
/// @param ignoreX If `true`, no x-axis torque is applied (e.g. when in a wheelie)
/// @details Adds the Y-component of `fLinear` to @ref m_totalForce.y. Computes the torque generated
/// by `fRot` at the given `pos` and adds it to @ref m_totalTorque, optionally ignoring the x-axis
/// component if `ignoreX` is `true`.
void KartDynamics::applySuspensionWrench(const EGG::Vector3f &pos, const EGG::Vector3f &fLinear,
        const EGG::Vector3f &fRot, bool ignoreX) {
    m_totalForce.y += fLinear.y;
    EGG::Vector3f fBody = m_fullRot.rotateVectorInv(fRot);
    EGG::Vector3f rBody = m_fullRot.rotateVectorInv(pos - m_pos);
    EGG::Vector3f torque = rBody.cross(fBody);

    if (ignoreX) {
        torque.x = 0.0f;
    }
    torque.y = 0.0f;
    m_totalTorque += torque;
}

/// @addr{0x805B6448}
/// @copybrief KartDynamics::stabilize()
/// @details Stabilizes the bike by rotating towards the target stabilization up vector using
/// spherical linear interpolation with a rate of @ref m_stabilizationFactor.
void KartDynamicsBike::stabilize() {
    EGG::Vector3f forward = m_up.cross(m_mainRot.rotateVector(EGG::Vector3f::ez)).cross(m_up);
    forward.normalise();
    EGG::Vector3f targetUp = forward.cross(m_stabilizeUp.cross(forward));
    targetUp.normalise();

    EGG::Vector3f top = m_mainRot.rotateVector(EGG::Vector3f::ey);
    if (EGG::Mathf::abs(top.dot(targetUp)) >= 0.9999f) {
        return;
    }

    EGG::Quatf q;
    q.makeVectorRotation(top, targetUp);
    m_mainRot = m_mainRot.slerpTo(q.multSwap(m_mainRot), m_stabilizationFactor);
}

} // namespace Kinoko::Kart
