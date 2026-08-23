#include "KartDynamics.hh"

namespace Kinoko::Kart {

/// @addr{0x805B4AF8}
KartDynamics::KartDynamics() {
    m_angVel0Factor = 1.0f;
    m_inertiaTensor = EGG::Matrix34f::ident;
    m_invInertiaTensor = EGG::Matrix34f::ident;
    init();
}

/// @addr{0x8059F6B8}
KartDynamics::~KartDynamics() = default;

/// @addr{0x805B5B68}
/// @details Rotates the kart up towards the world up vector
void KartDynamics::stabilize() {
    EGG::Vector3f top = m_mainRot.rotateVector(EGG::Vector3f::ey);
    if (EGG::Mathf::abs(top.dot(m_up)) >= 0.9999f) {
        return;
    }

    EGG::Quatf q;
    q.makeVectorRotation(top, m_up);
    m_mainRot = m_mainRot.slerpTo(q.multSwap(m_mainRot), m_stabilizationFactor);
}

/// @addr{0x805B4B54}
/// @brief Initializes the kart's dynamics to default values
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

/// @addr{0x805B4E84}
/// @brief Computes the intertia tensor from the provided cuboid dimensions
/// @param m The dimensions of the first cuboid with mas 12
/// @param n The dimensions of the second cuboid with mass 1
void KartDynamics::setInertia(const EGG::Vector3f &m, const EGG::Vector3f &n) {
    constexpr f32 TWELFTH = 1.0f / 12.0f;

    m_inertiaTensor = EGG::Matrix34f::zero;
    m_inertiaTensor[0, 0] = TWELFTH * (m.y * m.y + m.z * m.z) + (n.y * n.y + n.z * n.z);
    m_inertiaTensor[1, 1] = TWELFTH * (m.z * m.z + m.x * m.x) + (n.z * n.z + n.x * n.x);
    m_inertiaTensor[2, 2] = TWELFTH * (m.x * m.x + m.y * m.y) + (n.x * n.x + n.y * n.y);
    m_inertiaTensor.inverseTo33(m_invInertiaTensor);
}

/// @addr{0x805B5170}
/// @brief Every frame, computes acceleration, velocity, position, and rotation of the kart.
/// @param dt Delta time. It's always 1.0f.
/// @param maxSpeed Always 120.0f.
/// @param air True if the kart is in the air, false otherwise.
/// @details Applies acceleration from gravity and total force to external velocity. Decays external
/// velocity and angular velocity. Extracts the forward/backward component from external velocity
/// and stores it in @ref m_headingExtVel. If mid-air, caps the internal velocity at 120.0f.
/// Computes overall velocity from external, internal, and moving object/road velocities. Updates
/// position accordingly. Calculates overall angular velocity, stabilizes the vehicle, and applies
/// the resulting rotation.
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
/// @param ignoreX If true, no x-axis torque is applied (e.g. when in a wheelie)
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

/// @addr{0x805B5CE8}
/// @brief Applies a scaled linear force and rotationally to the kart.
/// @param pos Position of the rigid body.
/// @param fLinear Linear motion force
/// @param scale Scale factor for the torque
void KartDynamics::applyWrenchScaled(const EGG::Vector3f &pos, const EGG::Vector3f &fLinear,
        f32 scale) {
    m_totalForce += fLinear;

    EGG::Vector3f fBody = m_fullRot.rotateVectorInv(fLinear);
    EGG::Vector3f rBody = m_fullRot.rotateVectorInv(pos - m_pos);
    EGG::Vector3f torque = rBody.cross(fBody);

    m_totalTorque += torque * scale;
}

KartDynamicsBike::KartDynamicsBike() = default;

/// @addr{0x805B66E4}
KartDynamicsBike::~KartDynamicsBike() = default;

/// @addr{0x805B6448}
/// @details Stabilizes the bike by rotating towards the target stabilization up vector.
void KartDynamicsBike::stabilize() {
    EGG::Vector3f forward = m_up.cross(m_mainRot.rotateVector(EGG::Vector3f::ez)).cross(m_up);
    forward.normalise();
    EGG::Vector3f local_4c = forward.cross(m_stabilizeUp.cross(forward));
    local_4c.normalise();

    EGG::Vector3f top = m_mainRot.rotateVector(EGG::Vector3f::ey);
    if (EGG::Mathf::abs(top.dot(local_4c)) >= 0.9999f) {
        return;
    }

    EGG::Quatf q;
    q.makeVectorRotation(top, local_4c);
    m_mainRot = m_mainRot.slerpTo(q.multSwap(m_mainRot), m_stabilizationFactor);
}

} // namespace Kinoko::Kart
