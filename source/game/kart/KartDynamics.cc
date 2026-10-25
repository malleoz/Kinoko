#include "KartDynamics.hh"

namespace Kinoko::Kart {

/// @addr{0x805B4AF8}
/// @brief Constructor
/// @details Initializes @ref m_angVel0Factor to `1.0f` and @ref m_inertiaTensor and @ref
/// m_invInertiaTensor to the identity matrix. Lastly, calls @ref init() to finish
/// initialization.
KartDynamics::KartDynamics() {
    m_angVel0Factor = 1.0f;
    m_inertiaTensor = EGG::Matrix34f::ident;
    m_invInertiaTensor = EGG::Matrix34f::ident;
    init();
}

/// @addr{0x8059F6B8}
/// @brief Default virtual destructor
KartDynamics::~KartDynamics() = default;

/// @brief Called when the vehicle should ignore pitch changes due to torque and become upright
/// @details This is a nop for karts.
void KartDynamics::forceUpright() {}

/// @addr{0x805B5B68}
/// @brief Responsible for stabilizing the vehicle by rotating it upwards
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

/// @brief Resets the internal velocity to zero
void KartDynamics::resetInternalVelocity() {
    m_intVel.setZero();
}

/// @addr{0x805B4E84}
/// @brief Computes the intertia tensor from the provided cuboid dimensions
/// @param m The dimensions of the first cuboid with mass 12
/// @param n The dimensions of the second cuboid with mass 1
void KartDynamics::setInertia(const EGG::Vector3f &m, const EGG::Vector3f &n) {
    constexpr f32 TWELFTH = 1.0f / 12.0f;

    m_inertiaTensor = EGG::Matrix34f::zero;
    m_inertiaTensor[0, 0] = TWELFTH * (m.y * m.y + m.z * m.z) + (n.y * n.y + n.z * n.z);
    m_inertiaTensor[1, 1] = TWELFTH * (m.z * m.z + m.x * m.x) + (n.z * n.z + n.x * n.x);
    m_inertiaTensor[2, 2] = TWELFTH * (m.x * m.x + m.y * m.y) + (n.x * n.x + n.y * n.y);
    m_inertiaTensor.inverseTo33(m_invInertiaTensor);
}

/// @addr{0x805B4DC4}
/// @brief Takes elements from the kart's @ref BSP and computes the moment of inertia tensor
/// @param rotSpeed The rotational speed of the kart
/// @param m The dimensions of the first cuboid with mass 12
/// @param n The dimensions of the second cuboid with mass 1
/// @param skipInertia Whether to skip the inertia tensor calculation
/// @details Sets @ref m_angVel0Factor based on the provided `rotSpeed`. If `skipInertia` is
/// `false`, then calls @ref setIntertia() to compute the inertia tensor.
void KartDynamics::setBspParams(f32 rotSpeed, const EGG::Vector3f &m, const EGG::Vector3f &n,
        bool skipInertia) {
    m_angVel0Factor = rotSpeed;

    if (!skipInertia) {
        setInertia(m, n);
    }
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

    m_angVel0.x = std::clamp(m_angVel0.x, -0.4f, 0.4f);
    m_angVel0.y = std::clamp(m_angVel0.y, -0.4f, 0.4f) * m_angVel0YFactor;
    m_angVel0.z = std::clamp(m_angVel0.z, -0.8f, 0.8f);

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

/// @addr{0x805B5CE8}
/// @brief Applies a scaled linear force and rotationally to the kart.
/// @param pos Position of the rigid body.
/// @param fLinear Linear motion force
/// @param scale Scale factor for the torque
/// @details Adds `fLinear` to @ref m_totalForce. Computes the torque generated by `fLinear` at
/// the given `pos` and adds it to @ref m_totalTorque scaled by `scale`.
void KartDynamics::applyWrenchScaled(const EGG::Vector3f &pos, const EGG::Vector3f &fLinear,
        f32 scale) {
    m_totalForce += fLinear;

    EGG::Vector3f fBody = m_fullRot.rotateVectorInv(fLinear);
    EGG::Vector3f rBody = m_fullRot.rotateVectorInv(pos - m_pos);
    EGG::Vector3f torque = rBody.cross(fBody);

    m_totalTorque += torque * scale;
}

/// @addr{0x805B6388}
/// @brief Accumulates the provided force
/// @param force The force to accumulate
/// @details Simply adds `force` to @ref m_totalForce.
void KartDynamics::addForce(const EGG::Vector3f &force) {
    m_totalForce += force;
}

EGG::Matrix34f KartDynamics::calcWorldInertiaTensor() const {
    EGG::Matrix34f rotMat;
    rotMat.makeQ(m_mainRot);
    EGG::Matrix34f tmp = rotMat.multiplyTo(m_invInertiaTensor);
    return tmp.multiplyTo(rotMat.transpose());
}

/// @brief Sets the current position of the kart
/// @param pos The new position of the kart
void KartDynamics::setPos(const EGG::Vector3f &pos) {
    m_pos = pos;
}

/// @brief Sets the gravity affecting the kart
/// @param gravity The new gravity value
void KartDynamics::setGravity(f32 gravity) {
    m_gravity = gravity;
}

/// @brief Sets the main rotation quaternion of the kart
/// @param q The new main rotation quaternion of the kart
void KartDynamics::setMainRot(const EGG::Quatf &q) {
    m_mainRot = q;
}

/// @brief Sets the full rotation quaternion of the kart
/// @param q The new full rotation quaternion of the kart
void KartDynamics::setFullRot(const EGG::Quatf &q) {
    m_fullRot = q;
}

/// @brief Sets the stunt rotation quaternion of the kart
/// @param q The new stunt rotation quaternion of the kart
void KartDynamics::setStuntRot(const EGG::Quatf &q) {
    m_stuntRot = q;
}

/// @brief Sets the extra rotation quaternion of the kart
/// @param q The new extra rotation quaternion of the kart
void KartDynamics::setExtraRot(const EGG::Quatf &q) {
    m_extraRot = q;
}

/// @brief Sets the internal velocity of the kart
/// @param v The new internal velocity of the kart
void KartDynamics::setIntVel(const EGG::Vector3f &v) {
    m_intVel = v;
}

/// @brief Sets the up direction of the kart
/// @param v The new up direction of the kart
void KartDynamics::setUp(const EGG::Vector3f &v) {
    m_up = v;
}

/// @brief Sets the stabilization factor of the kart
/// @param val The new stabilization factor of the kart
void KartDynamics::setStabilizationFactor(f32 val) {
    m_stabilizationFactor = val;
}

/// @brief Sets the total force acting on the kart
/// @param v The new total force acting on the kart
void KartDynamics::setTotalForce(const EGG::Vector3f &v) {
    m_totalForce = v;
}

/// @brief Sets the external velocity of the kart
/// @param v The new external velocity of the kart
void KartDynamics::setExtVel(const EGG::Vector3f &v) {
    m_extVel = v;
}

/// @brief Sets the torque-related angular velocity of the kart
/// @param v The new torque-related angular velocity of the kart
void KartDynamics::setAngVel0(const EGG::Vector3f &v) {
    m_angVel0 = v;
}

/// @brief Sets the moving object velocity of the kart
/// @param v The new moving object velocity of the kart
void KartDynamics::setMovingObjVel(const EGG::Vector3f &v) {
    m_movingObjVel = v;
}

/// @brief Sets the moving road velocity of the kart
/// @param v The new moving road velocity of the kart
void KartDynamics::setMovingRoadVel(const EGG::Vector3f &v) {
    m_movingRoadVel = v;
}

/// @brief Sets the main angular velocity of the kart
/// @param v The new main angular velocity of the kart
void KartDynamics::setAngVel2(const EGG::Vector3f &v) {
    m_angVel2 = v;
}

/// @brief Sets the dampener for torque-related angular velocity
/// @param val The new dampener value for torque-related angular velocity
void KartDynamics::setAngVel0YFactor(f32 val) {
    m_angVel0YFactor = val;
}

/// @brief Sets the scale of the kart
/// @param v The new scale of the kart
void KartDynamics::setScale(const EGG::Vector3f &v) {
    m_scale = v;
}

/// @brief Sets the target up vector of the kart
/// @param v The new target up vector of the kart
void KartDynamics::setStabilizeUp(const EGG::Vector3f &v) {
    m_stabilizeUp = v;
}

/// @brief Sets whether the kart should be forced upright
/// @param isSet Whether to force the kart upright
void KartDynamics::setForceUpright(bool isSet) {
    m_forceUpright = isSet;
}

/// @brief Sets whether the kart is affected by gravity
/// @param isSet Whether to disable gravity for the kart
void KartDynamics::setNoGravity(bool isSet) {
    m_noGravity = isSet;
}

/// @brief Sets whether the Y component of the external velocity should be reset to zero
/// @param isSet Whether to reset the Y component of the external velocity
void KartDynamics::setKillExtVelY(bool isSet) {
    m_killExtVelY = isSet;
}

/// @brief Gets the inverse inertia tensor of the kart
/// @return The inverse inertia tensor of the kart, @ref m_invInertiaTensor
[[nodiscard]] const EGG::Matrix34f &KartDynamics::invInertiaTensor() const {
    return m_invInertiaTensor;
}

/// @brief Gets the dampener applied to @ref m_angVel0
/// @return The dampener applied for torque-related angular velocity, @ref m_angVel0Factor
[[nodiscard]] f32 KartDynamics::angVel0Factor() const {
    return m_angVel0Factor;
}

/// @brief Gets the position of the kart
/// @return The position of the kart, @ref m_pos
[[nodiscard]] const EGG::Vector3f &KartDynamics::pos() const {
    return m_pos;
}

/// @brief Gets the current velocity of the kart (the sum of internal and external velocities)
/// @return The current velocity of the kart, @ref m_velocity
[[nodiscard]] const EGG::Vector3f &KartDynamics::velocity() const {
    return m_velocity;
}

/// @brief Gets the current gravitational acceleration on the kart
/// @return The current gravitational acceleration on the kart, @ref m_gravity
[[nodiscard]] f32 KartDynamics::gravity() const {
    return m_gravity;
}

/// @brief Gets the internal velocity of the kart
/// @return The internal velocity of the kart, @ref m_intVel
[[nodiscard]] const EGG::Vector3f &KartDynamics::intVel() const {
    return m_intVel;
}

/// @brief Gets the main rotation quaternion of the kart
/// @return The main rotation quaternion of the kart, @ref m_mainRot
[[nodiscard]] const EGG::Quatf &KartDynamics::mainRot() const {
    return m_mainRot;
}

/// @brief Gets the full/combined rotation quaternion of the kart
/// @return The full rotation quaternion of the kart, @ref m_fullRot
[[nodiscard]] const EGG::Quatf &KartDynamics::fullRot() const {
    return m_fullRot;
}

/// @brief Gets the total force applied on the kart
/// @return The total force applied on the kart, @ref m_totalForce
[[nodiscard]] const EGG::Vector3f &KartDynamics::totalForce() const {
    return m_totalForce;
}

/// @brief Gets the external velocity of the kart
/// @return The external velocity of the kart, @ref m_extVel
[[nodiscard]] const EGG::Vector3f &KartDynamics::extVel() const {
    return m_extVel;
}

/// @brief Gets the angular velocity induced from @ref m_totalTorque
/// @return The angular velocity induced from torque, @ref m_angVel0
[[nodiscard]] const EGG::Vector3f &KartDynamics::angVel0() const {
    return m_angVel0;
}

/// @brief Gets the velocity induced from moving objects
/// @return The velocity induced from moving objects, @ref m_movingObjVel
[[nodiscard]] const EGG::Vector3f &KartDynamics::movingObjVel() const {
    return m_movingObjVel;
}

/// @brief Gets the velocity induced from moving water
/// @return The velocity induced from moving water, @ref m_movingRoadVel
[[nodiscard]] const EGG::Vector3f &KartDynamics::movingRoadVel() const {
    return m_movingRoadVel;
}

/// @brief Gets the main component of angular velocity
/// @return The main component of angular velocity, @ref m_angVel2
[[nodiscard]] const EGG::Vector3f &KartDynamics::angVel2() const {
    return m_angVel2;
}

/// @brief Gets the forward/backward component of @ref m_extVel
/// @return The forward/backward component of external velocity, @ref m_headingExtVel
[[nodiscard]] f32 KartDynamics::headingExtVel() const {
    return m_headingExtVel;
}

/// @brief Default constructor
KartDynamicsBike::KartDynamicsBike() = default;

/// @addr{0x805B66E4}
/// @brief Default virtual destructor
KartDynamicsBike::~KartDynamicsBike() = default;

/// @addr{0x805B6438}
/// @copybrief KartDynamics::forceUpright()
/// @details Clears the z-component of the angular velocity to force the bike upright
void KartDynamicsBike::forceUpright() {
    m_angVel0.z = 0.0f;
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
